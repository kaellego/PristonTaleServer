#include "Database/DatabaseManager.h"
#include "Database/SQLConnection.h"
#include "Config/ServerConfig.h"
#include "Logging/LogService.h"
#include <stdexcept>
#include <format>

const std::map<EDatabaseID, const char*> databaseMap = {
    { EDatabaseID::GameDB, "GameDB" },
    { EDatabaseID::LogDB, "LogDB" },
    { EDatabaseID::UserDB_Primary, "UserDB" }
};

DatabaseManager::DatabaseManager(const ServerConfig& config, LogService& logService)
    : m_logService(logService)
{
    m_logService.info("Inicializando gerenciador de banco de dados (modo Factory)...");
    const auto& dbConfig = config.getDatabaseConfig();

    // Pré-monta e armazena as strings de conexão
    for (const auto& pair : databaseMap) {
        EDatabaseID id = pair.first;
        const char* dbName = pair.second;

        m_connectionStrings[id] = std::format(
            "Driver={};Server={};Database={};Uid={};Pwd={};TrustServerCertificate=yes;",
            dbConfig.driver, dbConfig.host, dbName, dbConfig.user, dbConfig.password
        );
    }
    m_logService.info("Strings de conexao para {} bancos de dados foram preparadas.", m_connectionStrings.size());
}

DatabaseManager::~DatabaseManager() {
    m_logService.info("Encerrando gerenciador de banco de dados...");
}

// O novo método que atua como uma fábrica
std::unique_ptr<SQLConnection> DatabaseManager::createConnection(EDatabaseID id) {
    auto it = m_connectionStrings.find(id);
    if (it == m_connectionStrings.end()) {
        m_logService.error("Tentativa de criar conexao para um ID de banco de dados desconhecido: {}", static_cast<int>(id));
        return nullptr;
    }

    try {
        // Cria uma nova conexão usando a string armazenada
        return std::make_unique<SQLConnection>(it->second);
    }
    catch (const std::exception& e) {
        m_logService.error("Falha ao criar conexao sob demanda para o banco de dados ID {}: {}", static_cast<int>(id), e.what());
        return nullptr;
    }
}