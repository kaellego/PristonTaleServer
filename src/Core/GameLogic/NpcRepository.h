#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "GameLogic/GameWorld.h"

class DatabasePool;
class GameWorld;
class LogService;

class NpcRepository {
public:
    NpcRepository(DatabasePool& dbPool, LogService& logService)
        : m_dbPool(dbPool), m_logService(logService) {}

    // Loads only map instances with validated coordinates and Enabled=1.
    std::size_t loadEnabledSpawns(GameWorld& world) const;
    std::optional<NpcEntity> findEnabledDefinition(int npcId) const;
    bool saveGmSpawn(const NpcEntity& npc, const std::string& sourceMap) const;

private:
    DatabasePool& m_dbPool;
    LogService& m_logService;
};
