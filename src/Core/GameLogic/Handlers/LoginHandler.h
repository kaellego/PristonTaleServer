#pragma once
#include "Network/PacketDispatcher.h" // Para herdar de IPacketHandler

// Forward declarations
class AccountService;
class LogService;
struct GlobalState;

class LoginHandler : public IPacketHandler {
public:
    explicit LoginHandler(AccountService& accountService, LogService& logService, GlobalState& globalState);
    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;
private:
    AccountService& m_accountService;
    LogService& m_logService;
    GlobalState& m_globalState;
};