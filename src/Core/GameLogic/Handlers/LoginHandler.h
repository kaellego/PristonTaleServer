#pragma once
#include "Network/PacketDispatcher.h" // Para herdar de IPacketHandler

// Forward declarations
class AccountService;
class LogService;

class LoginHandler : public IPacketHandler {
public:
    explicit LoginHandler(AccountService& accountService, LogService& logService);
    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;
private:
    AccountService& m_accountService;
    LogService& m_logService;
};