#include "GameLogic/NpcRepository.h"

#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "GameLogic/GameWorld.h"
#include "Logging/LogService.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <string>

std::size_t NpcRepository::loadEnabledSpawns(GameWorld& world) const
{
    try
    {
        auto db = m_dbPool.createConnection(EDatabaseID::GameDB);
        db->prepare(R"SQL(
            SELECT s.SpawnId, s.NpcId, s.MapId, s.X, s.Y, s.Z, s.AngleY,
                   n.DisplayName, n.ModelName, n.EventCode, s.Enabled
              FROM dbo.NpcSpawn AS s
              INNER JOIN dbo.NpcList AS n ON n.NpcId = s.NpcId
             WHERE s.Enabled = 1 AND n.Enabled = 1
               AND s.X IS NOT NULL AND s.Y IS NOT NULL AND s.Z IS NOT NULL
        )SQL");
        db->execute();

        std::size_t loaded = 0;
        while (db->fetch())
        {
            const auto spawnId = db->getData<int>(1).value_or(0);
            const auto npcId = db->getData<int>(2).value_or(0);
            const auto mapId = db->getData<int>(3).value_or(-1);
            const auto x = db->getData<int>(4).value_or(0);
            const auto y = db->getData<int>(5).value_or(0);
            const auto z = db->getData<int>(6).value_or(0);
            const auto angleY = db->getData<int>(7).value_or(0);
            const auto name = db->getData<std::string>(8).value_or("");
            const auto model = db->getData<std::string>(9).value_or("");
            const auto eventCode = db->getData<int>(10).value_or(0);
            const auto enabled = db->getData<int>(11).value_or(0) != 0;

            if (spawnId <= 0 || npcId <= 0 || mapId < 0 || !enabled || name.empty() || model.empty() ||
                name.size() >= 32 || model.size() >= 64 || spawnId > 0x00FFFFFF)
                continue;

            NpcEntity npc{};
            npc.id = npcId;
            npc.objectSerial = 0x81000000u | static_cast<uint32_t>(spawnId);
            npc.mapId = mapId;
            npc.position = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
            npc.angleY = angleY;
            npc.name = name;
            npc.modelName = model;
            npc.eventCode = eventCode;
            npc.enabled = true;
            if (world.registerNpc(std::move(npc))) ++loaded;
        }

        m_logService.info("{} NPCs carregados do GameDB.", loaded);
        return loaded;
    }
    catch (const std::exception& error)
    {
        // The migration is optional during development; the safe fallback is
        // the small built-in catalog, never an unvalidated network spawn.
        m_logService.warn("Catalogo de NPCs indisponivel: {}", error.what());
        return 0;
    }
}
