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
        UserService& userService
    );

    void dispatch(std::shared_ptr<ClientSession> session, const Packet& packet);

private:
    void registerHandlers();

    // Referências para os serviços
    GlobalState& m_globalState;
    PlayerRepository& m_playerRepository;
    ItemRepository& m_itemRepository;
    AccountService& m_accountService;
    LogService& m_logService;
    UserService& m_userService;

    std::map<uint32_t, std::unique_ptr<IPacketHandler>> m_handlers;
};