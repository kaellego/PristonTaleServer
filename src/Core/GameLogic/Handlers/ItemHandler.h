#pragma once
#include "Network/PacketDispatcher.h"
class PlayerRepository;
class CharacterService;
class LogService;
class ItemHandler final : public IPacketHandler
{
public:
    ItemHandler(PlayerRepository &players, CharacterService &characters, LogService &log) : m_players(players), m_characters(characters), m_log(log) {}
    void handle(std::shared_ptr<ClientSession> session, const Packet &packet) override;

private:
    PlayerRepository &m_players;
    CharacterService &m_characters;
    LogService &m_log;
};
