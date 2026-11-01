#pragma once
#include "Network/PacketDispatcher.h"
class CharacterService;
class LogService;
class CharacterHandler final : public IPacketHandler
{
public:
    CharacterHandler(CharacterService &service, LogService &logService);
    void handle(std::shared_ptr<ClientSession> session, const Packet &packet) override;

private:
    CharacterService &m_service;
    LogService &m_logService;
};
