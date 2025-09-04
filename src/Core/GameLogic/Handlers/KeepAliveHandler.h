#pragma once
#include "Network/PacketDispatcher.h" // Para herdar de IPacketHandler

// Forward declarations
class LogService;

class KeepAliveHandler : public IPacketHandler {
public:
    explicit KeepAliveHandler(LogService& logService);
    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;
private:
    LogService& m_logService;
};