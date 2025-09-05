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

#include <iostream>

// Esta é a implementação que estava faltando para o vinculador.
PacketDispatcher::PacketDispatcher(
    GlobalState& state,
    PlayerRepository& playerRepo,
    ItemRepository& itemRepo,
    AccountService& accountService,
    LogService& logService,
    UserService& userService

) :
    m_globalState(state),
    m_playerRepository(playerRepo),
    m_itemRepository(itemRepo),
    m_accountService(accountService),
    m_logService(logService),
    m_userService(userService)
{
    m_logService.info("Registrando manipuladores de pacotes...");
    registerHandlers();
    m_logService.info("{} manipuladores de pacotes registrados.", m_handlers.size());
}

void PacketDispatcher::registerHandlers() {
    // Acessa os opcodes de forma segura e com escopo
    m_handlers[static_cast<uint32_t>(Opcodes::KeepAlive)] = std::make_unique<KeepAliveHandler>(m_logService);
    m_handlers[static_cast<uint32_t>(Opcodes::LoginUser)] = std::make_unique<LoginHandler>(m_accountService, m_logService, m_globalState);
    m_handlers[static_cast<uint32_t>(Opcodes::Ping)] = std::make_unique<PingHandler>(m_logService, m_userService, m_globalState);
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