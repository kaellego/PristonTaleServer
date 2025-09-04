#pragma once
#include <map>
#include <memory>
#include <string>

// Forward declarations
class ServerConfig;
class LogService;
class SQLConnection;

enum class EDatabaseID { GameDB, LogDB, UserDB_Primary, Invalid };

class DatabaseManager {
public:
    // Este � o construtor que Application.cpp ir� chamar
    explicit DatabaseManager(const ServerConfig& config, LogService& logService);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    SQLConnection* getConnection(EDatabaseID id);

private:
    LogService& m_logService;
    std::map<EDatabaseID, std::unique_ptr<SQLConnection>> m_connections;
};