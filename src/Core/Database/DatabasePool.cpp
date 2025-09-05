#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "Config/ServerConfig.h"
#include "Logging/LogService.h"
#include <stdexcept>
#include <format>

// Mapeamento de IDs para nomes de banco de dados
const std::map<EDatabaseID, const char*> databaseMap = {
    { EDatabaseID::GameDB, "GameDB" },
    { EDatabaseID::LogDB, "LogDB" },
    { EDatabaseID::UserDB_Primary, "UserDB" }
};

DatabasePool::DatabasePool(const ServerConfig& config, LogService& logService, size_t poolSizePerDb)
    : m_logService(logService)
{
    m_logService.info("Inicializando pool de conexoes de banco de dados...");
    const auto& dbConfig = config.getDatabaseConfig();

    for (const auto& pair : databaseMap) {
        EDatabaseID id = pair.first;
        const char* dbName = pair.second;

        m_logService.info("  - Criando pool para '{}' com {} conexoes...", dbName, poolSizePerDb);

        // Inicializa a pilha para este pool
        m_pools[id] = std::stack<std::unique_ptr<SQLConnection>>();

        for (size_t i = 0; i < poolSizePerDb; ++i) {
            try {
                std::string connStr = std::format(
                    "Driver={};Server={};Database={};Uid={};Pwd={};TrustServerCertificate=yes;",
                    dbConfig.driver, dbConfig.host, dbName, dbConfig.user, dbConfig.password
                );

                auto connection = std::make_unique<SQLConnection>(connStr);
                m_connectionToPoolMap[connection.get()] = id; // Mapeia o ponteiro de volta para o ID do pool
                m_pools[id].push(std::move(connection));

            }
            catch (const std::runtime_error& e) {
                throw std::runtime_error(std::format("Falha ao criar conexao #{} para o pool '{}': {}", i + 1, dbName, e.what()));
            }
        }
    }
    m_logService.info("Pool de conexoes de banco de dados inicializado com sucesso.");
}

DatabasePool::~DatabasePool() {
    m_logService.info("Encerrando pool de conexoes...");
    // A limpeza é automática devido aos unique_ptrs.
}

PooledConnection DatabasePool::getConnection(EDatabaseID id) {
    std::unique_lock<std::mutex> lock(m_mutex);

    // Espera até que o pool para o ID solicitado não esteja vazio
    m_condition.wait(lock, [this, id] { return !m_pools[id].empty(); });

    // Pega a conexão do topo da pilha
    std::unique_ptr<SQLConnection> connection = std::move(m_pools[id].top());
    m_pools[id].pop();

    // Retorna o invólucro RAII
    return PooledConnection(connection.release(), *this);
}

void DatabasePool::returnConnection(SQLConnection* connection) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Encontra a qual pool esta conexão pertence
        EDatabaseID id = m_connectionToPoolMap[connection];

        // Devolve a conexão ao pool
        m_pools[id].push(std::unique_ptr<SQLConnection>(connection));
    }
    // Notifica uma thread que está esperando que uma conexão está disponível
    m_condition.notify_one();
}


// --- Implementação do PooledConnection ---

PooledConnection::PooledConnection(SQLConnection* connection, DatabasePool& pool)
    : m_connection(connection), m_pool(&pool) {}

PooledConnection::~PooledConnection() {
    if (m_connection != nullptr) {
        m_pool->returnConnection(m_connection);
    }
}

PooledConnection::PooledConnection(PooledConnection&& other) noexcept
    : m_connection(other.m_connection), m_pool(other.m_pool) {
    // Impede que o destrutor do objeto movido devolva a conexão
    other.m_connection = nullptr;
    other.m_pool = nullptr;
}

PooledConnection& PooledConnection::operator=(PooledConnection&& other) noexcept {
    if (this != &other) {
        if (m_connection) {
            m_pool->returnConnection(m_connection);
        }
        m_connection = other.m_connection;
        m_pool = other.m_pool;
        other.m_connection = nullptr;
        other.m_pool = nullptr;
    }
    return *this;
}