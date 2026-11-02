#pragma once

#include <cstddef>

class DatabasePool;
class GameWorld;
class LogService;

class NpcRepository {
public:
    NpcRepository(DatabasePool& dbPool, LogService& logService)
        : m_dbPool(dbPool), m_logService(logService) {}

    // Loads only map instances with validated coordinates and Enabled=1.
    std::size_t loadEnabledSpawns(GameWorld& world) const;

private:
    DatabasePool& m_dbPool;
    LogService& m_logService;
};
