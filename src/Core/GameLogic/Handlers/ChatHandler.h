#pragma once

#include "Network/PacketDispatcher.h"

class GameWorld;
class LogService;

class ChatHandler final : public IPacketHandler {
public:
    ChatHandler(GameWorld& world, LogService& logService) : m_world(world), m_logService(logService) {}
    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;

private:
    GameWorld& m_world;
    LogService& m_logService;
};
