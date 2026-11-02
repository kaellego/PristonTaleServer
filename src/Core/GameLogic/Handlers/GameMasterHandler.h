#pragma once
#include "Network/Packet.h"
#include "Network/PacketDispatcher.h"
#include <memory>
class ClientSession;
class UserService;
class GameWorld;
class LogService;
class GameMasterHandler final : public IPacketHandler
{
public:
    GameMasterHandler(UserService &users, GameWorld &world, LogService &log) : m_users(users), m_world(world), m_log(log) {}
    void handle(std::shared_ptr<ClientSession> session, const Packet &packet) override;

private:
    UserService &m_users;
    GameWorld &m_world;
    LogService &m_log;
};
