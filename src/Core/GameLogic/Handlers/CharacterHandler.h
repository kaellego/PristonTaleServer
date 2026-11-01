#pragma once
#include "Network/PacketDispatcher.h"
class CharacterService;
class LogService;
class GameWorld;
class CharacterHandler final : public IPacketHandler
{
public:
    CharacterHandler(CharacterService &service, GameWorld& world, LogService &logService);
    void handle(std::shared_ptr<ClientSession> session, const Packet &packet) override;

private:
    CharacterService &m_service;
    GameWorld& m_world;
    LogService &m_logService;
};
