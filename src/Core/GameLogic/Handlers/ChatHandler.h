#pragma once

#include "Network/PacketDispatcher.h"

class GameWorld;
class LogService;
class UserService;
class NpcRepository;

class ChatHandler final : public IPacketHandler {
public:
    ChatHandler(GameWorld& world, UserService& users, NpcRepository& npcs, LogService& logService)
        : m_world(world), m_users(users), m_npcs(npcs), m_logService(logService) {}
    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;

private:
    GameWorld& m_world;
    UserService& m_users;
    NpcRepository& m_npcs;
    LogService& m_logService;
};
