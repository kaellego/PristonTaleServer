#include "GameLogic/Handlers/NpcHandler.h"

#include "GameLogic/GameWorld.h"
#include "Logging/LogService.h"
#include "Network/ClientSession.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Utils/SafeString.h"

#include <cmath>
#include <cstring>

void NpcHandler::handle(std::shared_ptr<ClientSession> session, const Packet &packet)
{
    if (!session || !session->isAuthenticated() || session->ticketExpired() ||
        session->getCharacterName().empty() || packet.body.size() != sizeof(PacketNpcClick))
    {
        if (session)
            session->close();
        return;
    }

    PacketNpcClick click{};
    std::memcpy(&click, packet.body.data(), sizeof(click));
    if (click.objectSerial == 0 || click.npcId < 0)
        return;
    const auto entityId = m_world.entityIdForAccount(session->getAccountName());
    if (!entityId)
        return;
    const auto npc = m_world.interactNpc(*entityId, click.npcId, click.objectSerial);
    if (!npc)
    {
        m_logService.warn("Interacao NPC rejeitada para {}: id={}, serial=0x{:08x}",
                          session->getAccountName(), click.npcId, click.objectSerial);
        return;
    }

    PacketNpcData data{};
    data.npcId = npc->id;
    data.objectSerial = npc->objectSerial;
    data.mapId = npc->mapId;
    data.x = static_cast<int32_t>(std::lround(npc->position.x));
    data.y = static_cast<int32_t>(std::lround(npc->position.y));
    data.z = static_cast<int32_t>(std::lround(npc->position.z));
    data.angleY = npc->angleY;
    data.eventCode = npc->eventCode;
    data.enabled = npc->enabled ? 1 : 0;
    copyFixed(data.szName, npc->name);
    copyFixed(data.szModelName, npc->modelName);
    const auto *dataBytes = reinterpret_cast<const uint8_t *>(&data);
    session->send(Packet(static_cast<uint32_t>(Opcodes::NpcData),
                         std::vector<uint8_t>(dataBytes, dataBytes + sizeof(data))));

    PacketNpcOpen opened{npc->id, npc->objectSerial, npc->eventCode, npc->mapId};
    const auto *bytes = reinterpret_cast<const uint8_t *>(&opened);
    session->send(Packet(static_cast<uint32_t>(Opcodes::OpenNPC),
                         std::vector<uint8_t>(bytes, bytes + sizeof(opened))));
    m_logService.info("NPC '{}' aberto para {} (id={})", npc->name, session->getAccountName(), npc->id);
}
