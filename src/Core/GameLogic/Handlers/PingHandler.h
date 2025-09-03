#pragma once
#include "Network/PacketDispatcher.h" // Para herdar de IPacketHandler

class PingHandler : public IPacketHandler {
public:
    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;
};