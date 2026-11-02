#include "GameLogic/Handlers/GameMasterHandler.h"
#include "GameLogic/Services/UserService.h"
#include "GameLogic/GameWorld.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Shared/datatypes.h"
#include <cstring>

namespace
{
    constexpr int kWarp = 0x100;
    constexpr int kResurrect = 0x130;
    constexpr int kSpawnNpc = 0x180;
}

void GameMasterHandler::handle(std::shared_ptr<ClientSession> session, const Packet &packet)
{
    if (!session->isAuthenticated() || session->ticketExpired() || packet.body.size() != 32 ||
        !m_users.isGameMaster(session->getAccountName()))
    {
        session->close();
        return;
    }
    int32_t p[8]{};
    std::memcpy(p, packet.body.data(), sizeof(p));
    bool ok = false;
    if (p[1] == kWarp)
        ok = m_world.gmWarp(session->getAccountName(), p[6], {static_cast<float>(p[0]), static_cast<float>(p[2]), static_cast<float>(p[3])});
    else if (p[1] == kResurrect)
        ok = m_world.gmHeal(session->getAccountName());
    else if (p[1] == kSpawnNpc)
    {
        auto npc = m_world.gmSpawnNpc(p[0], p[5], {static_cast<float>(p[2]), static_cast<float>(p[3]), static_cast<float>(p[4])});
        if (npc) {
            PacketNpcData data{};
            data.npcId = npc->id; data.objectSerial = npc->objectSerial; data.mapId = npc->mapId;
            data.x = static_cast<int32_t>(npc->position.x); data.y = static_cast<int32_t>(npc->position.y); data.z = static_cast<int32_t>(npc->position.z);
            data.eventCode = npc->eventCode; data.enabled = 1;
            std::memcpy(data.szName, npc->name.c_str(), npc->name.size());
            std::memcpy(data.szModelName, npc->modelName.c_str(), npc->modelName.size());
            const auto* bytes = reinterpret_cast<const uint8_t*>(&data);
            session->send(Packet(static_cast<uint32_t>(Opcodes::NpcData), std::vector<uint8_t>(bytes, bytes + sizeof(data))));
        }
        ok = npc.has_value();
    }
    else
    {
        m_log.warn("Comando GM desconhecido para {}: 0x{:x}", session->getAccountName(), p[1]);
        return;
    }
    m_log.info("Comando GM {} para {}: {}", p[1], session->getAccountName(), ok ? "aceito" : "rejeitado");
}
