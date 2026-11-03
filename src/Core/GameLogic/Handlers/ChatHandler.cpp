#include "GameLogic/Handlers/ChatHandler.h"
#include "GameLogic/GameWorld.h"
#include "GameLogic/NpcRepository.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "GameLogic/Services/UserService.h"
#include "Utils/SafeString.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

namespace
{
    void sendCommandReply(const std::shared_ptr<ClientSession> &client, const std::string &message,
                          uint32_t objectSerial = 0)
    {
        std::vector<uint8_t> body(sizeof(uint32_t) + 256, 0);
        std::memcpy(body.data(), &objectSerial, sizeof(objectSerial));
        const auto length = std::min(message.size(), body.size() - sizeof(uint32_t) - 1);
        std::memcpy(body.data() + sizeof(uint32_t), message.data(), length);
        client->send(Packet(static_cast<uint32_t>(Opcodes::ChatMessage), std::move(body)));
    }

    void sendNpcRefresh(const std::shared_ptr<ClientSession> &client, const GameWorld &world, int mapId)
    {
        for (const auto &npc : world.npcsForMap(mapId))
        {
            PacketNpcData data{};
            data.npcId = npc.id;
            data.objectSerial = npc.objectSerial;
            data.mapId = npc.mapId;
            data.x = static_cast<int32_t>(npc.position.x);
            data.y = static_cast<int32_t>(npc.position.y);
            data.z = static_cast<int32_t>(npc.position.z);
            data.angleY = npc.angleY;
            data.eventCode = npc.eventCode;
            data.enabled = npc.enabled ? 1 : 0;
            copyFixed(data.szName, npc.name);
            copyFixed(data.szModelName, npc.modelName);
            const auto *bytes = reinterpret_cast<const uint8_t *>(&data);
            const Packet npcPacket(static_cast<uint32_t>(Opcodes::NpcData),
                                   std::vector<uint8_t>(bytes, bytes + sizeof(data)));
            client->sendLegacyPacket(npcPacket, 0);
            // Reload must refresh an existing client slot even when its serial
            // is unchanged and the client therefore does not send GETPLAYERINFO again.
            client->sendLegacyPacket(npcPacket, 63);
        }
    }

    void sendNpc(const std::shared_ptr<ClientSession> &client, const NpcEntity &npc)
    {
        PacketNpcData data{};
        data.npcId = npc.id;
        data.objectSerial = npc.objectSerial;
        data.mapId = npc.mapId;
        data.x = static_cast<int32_t>(npc.position.x);
        data.y = static_cast<int32_t>(npc.position.y);
        data.z = static_cast<int32_t>(npc.position.z);
        data.angleY = npc.angleY;
        data.eventCode = npc.eventCode;
        data.enabled = npc.enabled ? 1 : 0;
        copyFixed(data.szName, npc.name);
        copyFixed(data.szModelName, npc.modelName);
        const auto *bytes = reinterpret_cast<const uint8_t *>(&data);
        const Packet npcPacket(static_cast<uint32_t>(Opcodes::NpcData),
                               std::vector<uint8_t>(bytes, bytes + sizeof(data)));
        client->sendLegacyPacket(npcPacket, 0);
        client->sendLegacyPacket(npcPacket, 63);
    }
}

void ChatHandler::handle(std::shared_ptr<ClientSession> session, const Packet &packet)
{
    if (!session->isAuthenticated() || session->ticketExpired() ||
        packet.body.size() != sizeof(uint32_t) + 256)
        return;

    if (std::memchr(packet.body.data() + sizeof(uint32_t), '\0', 256) == nullptr)
        return;

    const auto *text = reinterpret_cast<const char *>(packet.body.data() + sizeof(uint32_t));
    std::istringstream commandLine(std::string(text, strnlen_s(text, 256)));
    std::string command;
    commandLine >> command;
    m_logService.debug("Chat recebido de {}: '{}'", session->getAccountName(), command);
    if (command == "/reloadnpc" || command == "/reloadnpcs")
    {
        if (!m_users.isGameMaster(session->getAccountName()))
        {
            m_logService.warn("Reload de NPCs rejeitado: {} nao e GM", session->getAccountName());
            sendCommandReply(session, "[GM] ERRO: comando permitido somente para GM.", session->clientObjectSerial());
            return;
        }
        const auto loaded = m_npcs.loadEnabledSpawns(m_world);
        std::size_t recipients = 0;
        for (const auto &[client, mapId] : m_world.clientsWithMap())
        {
            sendNpcRefresh(client, m_world, mapId);
            ++recipients;
        }
        m_logService.info("Reload de NPCs executado por {}: {} NPCs, {} sessoes atualizadas",
                          session->getAccountName(), loaded, recipients);
        sendCommandReply(session, "[GM] OK: NPCs recarregados (" + std::to_string(loaded) + ").",
                         session->clientObjectSerial());
        return;
    }
    if (command == "/spawnnpc")
    {
        if (!m_users.isGameMaster(session->getAccountName()))
        {
            m_logService.warn("Spawn de NPC rejeitado: {} nao e GM", session->getAccountName());
            sendCommandReply(session, "[GM] ERRO: comando permitido somente para GM.", session->clientObjectSerial());
            return;
        }
        int npcId = 0;
        if (!(commandLine >> npcId) || npcId <= 0)
        {
            m_logService.warn("Spawn de NPC rejeitado para {}: ID invalido", session->getAccountName());
            sendCommandReply(session, "[GM] ERRO: informe um ID de NPC valido.", session->clientObjectSerial());
            return;
        }
        const auto entity = m_world.entityForAccount(session->getAccountName());
        const auto definition = m_npcs.findEnabledDefinition(npcId);
        if (!entity || !definition)
        {
            m_logService.warn("Spawn de NPC rejeitado para {}: id={} sem definicao ou personagem ativo",
                              session->getAccountName(), npcId);
            sendCommandReply(session, "[GM] ERRO: NPC inexistente ou personagem fora do mapa.",
                             session->clientObjectSerial());
            return;
        }
        const auto npc = m_world.gmSpawnNpc(*definition, entity->mapId, entity->position);
        if (!npc)
        {
            m_logService.warn("Spawn de NPC rejeitado para {}: id={} nao pode ser registrado",
                              session->getAccountName(), npcId);
            sendCommandReply(session, "[GM] ERRO: nao foi possivel criar o NPC.", session->clientObjectSerial());
            return;
        }
        const auto sourceMap = "gm:" + session->getAccountName() + ":" + std::to_string(npcId);
        const bool persisted = m_npcs.saveGmSpawn(*npc, sourceMap);
        std::size_t recipients = 0;
        for (const auto &[client, mapId] : m_world.clientsWithMap())
        {
            if (mapId == npc->mapId)
            {
                sendNpc(client, *npc);
                ++recipients;
            }
        }
        m_logService.info("NPC {} criado por {} em mapa={} x={} y={} z={} para {} sessoes",
                          npcId, session->getAccountName(), npc->mapId, npc->position.x,
                          npc->position.y, npc->position.z, recipients);
        const auto reply = persisted
            ? "[GM] OK: NPC " + std::to_string(npcId) + " criado e salvo."
            : "[GM] AVISO: NPC criado, mas nao foi salvo no banco.";
        sendCommandReply(session, reply,
                         session->clientObjectSerial());
        return;
    }
    if (command == "/pos" || command == "/coords" || command == "/coordenadas")
    {
        if (!m_users.isGameMaster(session->getAccountName()))
        {
            m_logService.warn("Comando de coordenadas rejeitado: {} nao e GM", session->getAccountName());
            sendCommandReply(session, "[GM] ERRO: comando permitido somente para GM.", session->clientObjectSerial());
            return;
        }
        const auto entity = m_world.entityForAccount(session->getAccountName());
        if (!entity)
        {
            m_logService.warn("Comando de coordenadas rejeitado: {} sem entidade no GameWorld", session->getAccountName());
            sendCommandReply(session, "[GM] ERRO: personagem ainda nao esta no mapa.", session->clientObjectSerial());
            return;
        }
        char response[256]{};
        std::snprintf(response, sizeof(response), "[GM] Mapa %d | X %.0f Y %.0f Z %.0f",
                      entity->mapId, entity->position.x, entity->position.y, entity->position.z);
        const auto objectSerial = session->clientObjectSerial() != 0 ? session->clientObjectSerial() : entity->objectSerial;
        sendCommandReply(session, response, objectSerial);
        m_logService.info("Coordenadas GM solicitadas por {}: mapa={} x={} y={} z={}",
                          session->getAccountName(), entity->mapId, entity->position.x,
                          entity->position.y, entity->position.z);
        return;
    }
    const auto entityId = m_world.entityIdForAccount(session->getAccountName());
    if (!entityId)
        return;
    m_world.broadcastNearby(session->getAccountName(), *entityId, packet);
    m_logService.debug("Chat {}: {}", session->getCharacterName(), text);
}
