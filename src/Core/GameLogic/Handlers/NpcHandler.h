#pragma once

#include "Network/PacketDispatcher.h"

class GameWorld;
class LogService;

class NpcHandler final : public IPacketHandler {
public:
    NpcHandler(GameWorld& world, LogService& logService) : m_world(world), m_logService(logService) {}
    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;

private:
    GameWorld& m_world;
    LogService& m_logService;
};
