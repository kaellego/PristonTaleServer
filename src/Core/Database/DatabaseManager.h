#pragma once
#include <map>
#include <memory>
#include <string>

// Forward declarations
class ServerConfig;
class LogService;
class SQLConnection;

enum class EDatabaseID { GameDB, LogDB, UserDB_Primary, Invalid };

/**
 * @class DatabaseManager
 * @brief Atua como uma F�BRICA para criar conex�es de banco de dados sob demanda.
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const ServerConfig& config, LogService& logService);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief Cria e retorna uma nova e exclusiva conex�o de banco de dados.
     * @param id O identificador do banco de dados ao qual se conectar.
     * @return Um std::unique_ptr para o objeto SQLConnection.
     */
    std::unique_ptr<SQLConnection> createConnection(EDatabaseID id);

private:
    LogService& m_logService;
    // Armazena as STRINGS de conex�o, n�o mais os objetos de conex�o
    std::map<EDatabaseID, std::string> m_connectionStrings;
};