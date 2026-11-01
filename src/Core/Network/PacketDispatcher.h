#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <functional>

// Forward declarations
class ClientSession;
struct Packet;
struct GlobalState;
class PlayerRepository;
class ItemRepository;
class AccountService;
class LogService;
class UserService;
class CharacterService;
class InventoryService;
class GameWorld;

class IPacketHandler {
public:
    virtual ~IPacketHandler() = default;
    virtual void handle(std::shared_ptr<ClientSession> session, const Packet& packet) = 0;
};

class PacketDispatcher {
public:
    // Este é o construtor que o Application.cpp está tentando chamar
    explicit PacketDispatcher(
        GlobalState& state,
        PlayerRepository& playerRepo,
        ItemRepository& itemRepo,
        AccountService& accountService,
        LogService& logService,
        UserService& userService,
        CharacterService& characterService,
        InventoryService& inventoryService,
        GameWorld& gameWorld
    );

    void dispatch(std::shared_ptr<ClientSession> session, const Packet& packet);
    void sessionClosed(std::shared_ptr<ClientSession> session);

private:
    void registerHandlers();

    // Referências para os serviços
    GlobalState& m_globalState;
    PlayerRepository& m_playerRepository;
    ItemRepository& m_itemRepository;
    AccountService& m_accountService;
    LogService& m_logService;
    UserService& m_userService;
    CharacterService& m_characterService;
    InventoryService& m_inventoryService;
    GameWorld& m_gameWorld;

    std::map<uint32_t, std::shared_ptr<IPacketHandler>> m_handlers;
};
