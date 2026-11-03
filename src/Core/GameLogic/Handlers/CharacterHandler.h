#pragma once
#include "Network/PacketDispatcher.h"
class CharacterService;
class LogService;
class GameWorld;
class UserService;
class CharacterHandler final : public IPacketHandler
{
public:
    CharacterHandler(CharacterService &service, GameWorld& world, UserService& userService, LogService &logService);
    void handle(std::shared_ptr<ClientSession> session, const Packet &packet) override;

private:
    CharacterService &m_service;
    GameWorld& m_world;
    UserService& m_userService;
    LogService &m_logService;
};
