#pragma once
#include "Database/DatabaseDialect.h"
#include <string>
#include <cstddef>
#include <memory>

class ServerConfig;
class LogService;
class SQLConnection;
enum class EDatabaseID { GameDB, LogDB, UserDB_Primary, Invalid };

class DatabasePool {
public:
    explicit DatabasePool(const ServerConfig& config, LogService& logService, size_t poolSizePerDb = 5);
    DatabasePool(const DatabasePool&) = delete;
    DatabasePool& operator=(const DatabasePool&) = delete;
    DatabaseDialect dialect() const noexcept { return m_dialect; }
    std::string connectionString(EDatabaseID id) const;
    std::unique_ptr<SQLConnection> createConnection(EDatabaseID id) const;
private:
    LogService& m_logService;
    DatabaseDialect m_dialect = DatabaseDialect::Unknown;
    std::string m_driver, m_host, m_user, m_password;
};
