#pragma once

#include "Database/DatabaseDialect.h"
#include "Shared/datatypes.h"
#include <optional>
#include <string>
#include <vector>

class DatabasePool;
class LogService;

class Orm {
public:
    Orm(DatabasePool& pool, LogService& log, DatabaseDialect dialect);

    std::optional<SQLUser> findUser(const std::string& accountName) const;
    std::vector<std::string> findCharacterNames(const std::string& accountName, int limit = 6) const;
    bool characterBelongsTo(const std::string& accountName, const std::string& characterName) const;
    bool createCharacterRecord(const std::string& accountName, const std::string& characterName) const;
    bool deleteCharacterRecord(const std::string& accountName, const std::string& characterName) const;
    DatabaseDialect dialect() const noexcept { return m_dialect; }

private:
    DatabasePool& m_pool;
    LogService& m_log;
    DatabaseDialect m_dialect;
};
