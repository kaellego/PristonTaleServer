#include "GameLogic/NpcRepository.h"

#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "GameLogic/GameWorld.h"
#include "Logging/LogService.h"

#include <cstdint>
#include <exception>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace {
std::string clientPatternPath(std::string model)
{
    if (model.empty())
        return {};

    // The legacy client decodes the .inx model index, which points to the
    // cached .smd mesh. Sending .ase makes smModelDecode skip the index and
    // fails when the client distribution does not contain the source mesh.
    const auto extension = model.find_last_of('.');
    if (extension == std::string::npos)
        model += ".inx";
    else
        model.replace(extension, std::string::npos, ".inx");
    return model;
}
}

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

        std::vector<NpcEntity> npcs;
        std::size_t rowNumber = 0;
        std::size_t skipped = 0;
        while (db->fetch())
        {
            ++rowNumber;
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
            {
                ++skipped;
                m_logService.warn("NPC ignorado na linha {}: spawn={}, npc={}, mapa={}, enabled={}, nome='{}', modelo='{}'",
                                  rowNumber, spawnId, npcId, mapId, enabled ? 1 : 0, name, model);
                continue;
            }

            NpcEntity npc{};
            npc.id = npcId;
            npc.objectSerial = 0x81000000u | static_cast<uint32_t>(spawnId);
            npc.mapId = mapId;
            // NpcSpawn already stores the fixed-point coordinates read from
            // the legacy map data. Keep them unchanged; multiplying here
            // moves the NPC hundreds of millions of units away from the map.
            npc.position = {static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(z)};
            npc.angleY = angleY;
            npc.name = name;
            npc.modelName = clientPatternPath(model);
            npc.eventCode = eventCode;
            npc.enabled = true;
            m_logService.debug("NPC carregado: spawn={} id={} serial=0x{:08x} mapa={} pos=({}, {}, {}) angulo={} evento={} nome='{}' modelo='{}'",
                               spawnId, npc.id, npc.objectSerial, npc.mapId, x, y, z, npc.angleY,
                               npc.eventCode, npc.name, npc.modelName);
            npcs.push_back(std::move(npc));
        }

        const auto loaded = world.replaceNpcs(std::move(npcs));
        m_logService.info("{} NPCs carregados do GameDB; {} registros ignorados.", loaded, skipped);
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

std::optional<NpcEntity> NpcRepository::findEnabledDefinition(int npcId) const
{
    if (npcId <= 0) return std::nullopt;
    try
    {
        auto db = m_dbPool.createConnection(EDatabaseID::GameDB);
        db->prepare(R"SQL(
            SELECT NpcId, DisplayName, ModelName, EventCode
              FROM dbo.NpcList
             WHERE NpcId = ? AND Enabled = 1
        )SQL");
        db->bindParameter(1, npcId);
        db->execute();
        if (!db->fetch()) return std::nullopt;

        NpcEntity npc{};
        npc.id = db->getData<int>(1).value_or(0);
        npc.name = db->getData<std::string>(2).value_or("");
        const auto rawModel = db->getData<std::string>(3).value_or("");
        npc.modelName = clientPatternPath(rawModel);
        npc.eventCode = db->getData<int>(4).value_or(0);
        if (npc.id <= 0 || npc.name.empty() || npc.modelName.empty() || npc.modelName == ".inx") {
            m_logService.warn("Definicao de NPC rejeitada: id={} nome='{}' modeloBanco='{}' modeloCliente='{}'",
                              npc.id, npc.name, rawModel, npc.modelName);
            return std::nullopt;
        }
        npc.enabled = true;
        return npc;
    }
    catch (const std::exception& error)
    {
        m_logService.warn("Consulta do NPC {} rejeitada: {}", npcId, error.what());
        return std::nullopt;
    }
}

bool NpcRepository::saveGmSpawn(const NpcEntity& npc, const std::string& sourceMap) const
{
    if (npc.id <= 0 || npc.mapId < 0 || sourceMap.empty() || sourceMap.size() >= 128 ||
        !std::isfinite(npc.position.x) || !std::isfinite(npc.position.y) || !std::isfinite(npc.position.z) ||
        npc.position.x < std::numeric_limits<int32_t>::min() || npc.position.x > std::numeric_limits<int32_t>::max() ||
        npc.position.y < std::numeric_limits<int32_t>::min() || npc.position.y > std::numeric_limits<int32_t>::max() ||
        npc.position.z < std::numeric_limits<int32_t>::min() || npc.position.z > std::numeric_limits<int32_t>::max())
        return false;

    const auto x = static_cast<int32_t>(std::lround(npc.position.x));
    const auto y = static_cast<int32_t>(std::lround(npc.position.y));
    const auto z = static_cast<int32_t>(std::lround(npc.position.z));

    try
    {
        auto db = m_dbPool.createConnection(EDatabaseID::GameDB);
        db->beginTransaction();
        try
        {
            db->prepare(R"SQL(
                IF EXISTS (
                    SELECT 1 FROM dbo.NpcSpawn WITH (UPDLOCK, HOLDLOCK)
                     WHERE NpcId = ? AND MapId = ? AND SourceMap = ?
                )
                BEGIN
                    UPDATE dbo.NpcSpawn
                       SET X = ?, Y = ?, Z = ?, AngleY = ?, Enabled = 1
                     WHERE NpcId = ? AND MapId = ? AND SourceMap = ?;
                END
                ELSE
                BEGIN
                    INSERT INTO dbo.NpcSpawn
                        (NpcId, MapId, SourceMap, X, Y, Z, AngleY, Enabled)
                    VALUES (?, ?, ?, ?, ?, ?, ?, 1);
                END
            )SQL");
            db->bindParameter(1, npc.id);
            db->bindParameter(2, npc.mapId);
            db->bindParameter(3, sourceMap);
            db->bindParameter(4, x);
            db->bindParameter(5, y);
            db->bindParameter(6, z);
            db->bindParameter(7, npc.angleY);
            db->bindParameter(8, npc.id);
            db->bindParameter(9, npc.mapId);
            db->bindParameter(10, sourceMap);
            db->bindParameter(11, npc.id);
            db->bindParameter(12, npc.mapId);
            db->bindParameter(13, sourceMap);
            db->bindParameter(14, x);
            db->bindParameter(15, y);
            db->bindParameter(16, z);
            db->bindParameter(17, npc.angleY);
            db->execute();
            db->commit();
            m_logService.info("Spawn GM persistido: npc={} mapa={} pos=({}, {}, {}) origem='{}'",
                              npc.id, npc.mapId, x, y, z, sourceMap);
            return true;
        }
        catch (...)
        {
            db->rollback();
            throw;
        }
    }
    catch (const std::exception& error)
    {
        m_logService.error("Falha ao persistir spawn GM npc={} mapa={} origem='{}': {}",
                           npc.id, npc.mapId, sourceMap, error.what());
        return false;
    }
}
