#include "Network/PacketDispatcher.h"
#include "Network/Packet.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Shared/Constants.h"
#include "Shared/GlobalState.h"

#include "GameLogic/Services/AccountService.h"
#include "GameLogic/Services/UserService.h"

#include "GameLogic/Handlers/KeepAliveHandler.h"
#include "GameLogic/Handlers/LoginHandler.h"
#include "GameLogic/Handlers/PingHandler.h"
#include "GameLogic/Handlers/CharacterHandler.h"
#include "GameLogic/Handlers/ItemHandler.h"
#include "GameLogic/Handlers/MovementHandler.h"
#include "GameLogic/Handlers/CombatHandler.h"
#include "GameLogic/Handlers/ChatHandler.h"
#include "GameLogic/Handlers/NpcHandler.h"
#include "GameLogic/Services/CharacterService.h"
#include "GameLogic/Services/InventoryService.h"
#include "GameLogic/GameWorld.h"

#include <iostream>

// Esta é a implementação que estava faltando para o vinculador.
PacketDispatcher::PacketDispatcher(
    GlobalState& state,
    PlayerRepository& playerRepo,
    ItemRepository& itemRepo,
    AccountService& accountService,
    LogService& logService,
    UserService& userService,
    CharacterService& characterService,
    InventoryService& inventoryService,
    GameWorld& gameWorld

) :
    m_globalState(state),
    m_playerRepository(playerRepo),
    m_itemRepository(itemRepo),
    m_accountService(accountService),
    m_logService(logService),
    m_userService(userService),
    m_characterService(characterService),
    m_inventoryService(inventoryService),
    m_gameWorld(gameWorld)
{
    m_logService.info("Registrando manipuladores de pacotes...");
    registerHandlers();
    m_logService.info("{} manipuladores de pacotes registrados.", m_handlers.size());
}

void PacketDispatcher::registerHandlers() {
    // Acessa os opcodes de forma segura e com escopo
    m_handlers[static_cast<uint32_t>(Opcodes::KeepAlive)] = std::make_shared<KeepAliveHandler>(m_logService);
    m_handlers[static_cast<uint32_t>(Opcodes::LegacyNoop)] = m_handlers[static_cast<uint32_t>(Opcodes::KeepAlive)];
    m_handlers[static_cast<uint32_t>(Opcodes::LegacyConnected)] = m_handlers[static_cast<uint32_t>(Opcodes::KeepAlive)];
    m_handlers[static_cast<uint32_t>(Opcodes::LoginUser)] = std::make_shared<LoginHandler>(m_accountService, m_logService, m_globalState);
    m_handlers[static_cast<uint32_t>(Opcodes::Ping)] = std::make_shared<PingHandler>(m_logService, m_userService, m_globalState);
    auto characterHandler = std::make_shared<CharacterHandler>(m_characterService, m_gameWorld, m_logService);
    m_handlers[static_cast<uint32_t>(Opcodes::SelectCharacter)] = characterHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::CreateCharacter)] = characterHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::DeleteCharacter)] = characterHandler;
    auto itemHandler = std::make_shared<ItemHandler>(m_inventoryService, m_gameWorld, m_logService);
    m_handlers[static_cast<uint32_t>(Opcodes::NetPlayerInventory)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::NetOpenWarehouse)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::NetOpenWarehouseSeasonal)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::RecvWarehouse)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::SendWarehouse)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::NetWarehouseTransfer)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::NetDropItem)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::NetPickupDrop)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::ItemSlotHandle)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::UseItemCode)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::CheckItemUseSlot)] = itemHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::CheckItemUseSlotEx)] = itemHandler;
    auto movementHandler = std::make_shared<MovementHandler>(m_gameWorld, m_logService);
    m_handlers[static_cast<uint32_t>(Opcodes::UnitStatusMove)] = movementHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::PlayData)] = movementHandler;
    auto combatHandler = std::make_shared<CombatHandler>(m_gameWorld, m_logService);
    m_handlers[static_cast<uint32_t>(Opcodes::AttackData)] = combatHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::AttackData2)] = combatHandler;
    m_handlers[static_cast<uint32_t>(Opcodes::ChatMessage)] = std::make_shared<ChatHandler>(m_gameWorld, m_logService);
    m_handlers[static_cast<uint32_t>(Opcodes::NpcUnitClick)] = std::make_shared<NpcHandler>(m_gameWorld, m_logService);
    // Adicione outros handlers aqui conforme for implementando...
    // Ex: m_handlers[static_cast<uint32_t>(Opcodes::SelectCharacter)] = std::make_unique<SelectCharacterHandler>(...);
}

void PacketDispatcher::dispatch(std::shared_ptr<ClientSession> session, const Packet& packet) {
    uint32_t opcode = packet.header.opcode;
    auto it = m_handlers.find(opcode);

    if (it != m_handlers.end()) {
        it->second->handle(session, packet);
    }
    else {
        std::string client_ip = "IP desconhecido";
        try {
            client_ip = session->socket().remote_endpoint().address().to_string();
        }
        catch (...) {}

        m_logService.warn("Pacote nao manipulado recebido do cliente {}. Opcode: 0x{:04x}", client_ip, opcode);
    }
}

bool PacketDispatcher::transferLegacySession(std::shared_ptr<ClientSession> session, const std::string& accountName) {
    return m_userService.transferSession(accountName, session);
}

bool PacketDispatcher::transferLegacySessionByIp(std::shared_ptr<ClientSession> session) {
    return m_userService.transferSessionByIp(session);
}

void PacketDispatcher::sessionClosed(std::shared_ptr<ClientSession> session) {
    m_gameWorld.leave(session->getAccountName());
    m_userService.removeUser(std::move(session));
}
