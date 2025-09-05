#pragma once
#include "Network/PacketDispatcher.h" // Para herdar de IPacketHandler

// Forward declarations
class LogService;
class UserService;
struct GlobalState;

class PingHandler : public IPacketHandler {
public:
    // O construtor recebe as dependências de que precisa
    explicit PingHandler(LogService& logService, UserService& userService, GlobalState& globalState);

    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;

private:
    LogService& m_logService;
    UserService& m_userService;
    GlobalState& m_globalState;
};