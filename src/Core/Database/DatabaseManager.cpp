#include "Database/DatabaseManager.h"
#include "Database/SQLConnection.h"
#include "Config/ServerConfig.h"
#include "Logging/LogService.h"
#include <stdexcept>
#include <format> // Requer C++20

// Mapeamento interno de IDs para nomes de banco de dados no SQL Server
const std::map<EDatabaseID, const char*> databaseMap = {
    { EDatabaseID::GameDB, "GameDB" },
    { EDatabaseID::LogDB, "LogDB" },
    { EDatabaseID::UserDB_Primary, "UserDB" }
    // Adicione todos os outros EDatabaseID e seus nomes de DB correspondentes aqui
};

DatabaseManager::DatabaseManager(const ServerConfig& config, LogService& logService)
    : m_logService(logService)
{
    m_logService.info("Inicializando gerenciador de banco de dados...");

    const auto& dbConfig = config.getDatabaseConfig();

    for (const auto& pair : databaseMap) {
        EDatabaseID id = pair.first;
        const char* dbName = pair.second;

        try {
            // Constrói a string de conexão ODBC para cada banco de dados
            std::string connStr = std::format(
                "Driver={};Server={};Database={};Uid={};Pwd={};TrustServerCertificate=yes;",
                dbConfig.driver, dbConfig.host, dbName, dbConfig.user, dbConfig.password
            );

            // Cria a conexão. Se falhar, o construtor da SQLConnection lança uma exceção.
            auto connection = std::make_unique<SQLConnection>(connStr);
            m_logService.info("  - Conexao para '{}' estabelecida com sucesso.", dbName);

            m_connections[id] = std::move(connection);

        }
        catch (const std::runtime_error& e) {
            // Propaga o erro fatal para a classe Application
            throw std::runtime_error(std::format("Falha ao criar conexao para '{}': {}", dbName, e.what()));
        }
    }
    m_logService.info("Gerenciador de banco de dados inicializado.");
}

DatabaseManager::~DatabaseManager() {
    m_logService.info("Encerrando gerenciador de banco de dados...");
    m_connections.clear();
}

SQLConnection* DatabaseManager::getConnection(EDatabaseID id) {
    auto it = m_connections.find(id);
    if (it != m_connections.end()) {
        return it->second.get();
    }
    m_logService.error("Tentativa de obter conexao de banco de dados invalida: {}", static_cast<int>(id));
    return nullptr;
}