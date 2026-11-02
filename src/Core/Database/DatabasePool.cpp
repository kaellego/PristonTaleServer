#include "Database/DatabasePool.h"
#include "Config/ServerConfig.h"
#include "Logging/LogService.h"
#include "Database/SQLConnection.h"
#include <format>
#include <stdexcept>

namespace { const char* databaseName(EDatabaseID id) {
    switch (id) { case EDatabaseID::GameDB: return "GameDB"; case EDatabaseID::LogDB: return "LogDB"; case EDatabaseID::UserDB_Primary: return "UserDB"; default: return nullptr; }
} }

DatabasePool::DatabasePool(const ServerConfig& config, LogService& logService, size_t poolSizePerDb) : m_logService(logService) {
    if (poolSizePerDb == 0) throw std::invalid_argument("O pool de banco deve ter pelo menos uma conexao por banco.");
    const auto& db = config.getDatabaseConfig();
    m_driver = db.driver; m_host = db.host; m_user = db.user; m_password = db.password;
    m_dialect = detectDatabaseDialect(m_driver);
    if (m_dialect == DatabaseDialect::Unknown) throw std::runtime_error("Driver ODBC nao suportado: " + m_driver);
    for (const auto database : { EDatabaseID::GameDB, EDatabaseID::LogDB, EDatabaseID::UserDB_Primary }) {
        auto& state = m_pools[poolIndex(database)];
        state.owned.reserve(poolSizePerDb);
        for (size_t i = 0; i < poolSizePerDb; ++i) {
            state.owned.push_back(std::make_unique<SQLConnection>(connectionString(database)));
            state.available.push_back(state.owned.back().get());
        }
    }
    m_logService.info("Pool ODBC inicializado: {} conexoes por banco; acesso limitado por fila.", poolSizePerDb);
}

std::string DatabasePool::connectionString(EDatabaseID id) const {
    const auto* name = databaseName(id); if (!name) return {};
    auto driver = m_driver;
    if (driver.size() > 1 && driver.front() == '{' && driver.back() == '}') driver = driver.substr(1, driver.size() - 2);
    m_logService.debug("SOCI ODBC driver='{}' host='{}' database='{}'", driver, m_host, name);
    return std::format("DRIVER={{{}}};SERVER={},1433;DATABASE={};UID={};PWD={};TrustServerCertificate=yes;", driver, m_host, name, m_user, m_password);
}

std::size_t DatabasePool::poolIndex(EDatabaseID id) {
    switch (id) {
    case EDatabaseID::GameDB: return 0;
    case EDatabaseID::LogDB: return 1;
    case EDatabaseID::UserDB_Primary: return 2;
    default: throw std::invalid_argument("Banco invalido para o pool.");
    }
}

DatabasePool::Connection DatabasePool::createConnection(EDatabaseID id) const {
    auto& state = m_pools[poolIndex(id)];
    std::unique_lock lock(state.mutex);
    state.availableCondition.wait(lock, [&state] { return !state.available.empty(); });
    auto* connection = state.available.front();
    state.available.pop_front();
    return Connection(connection, ConnectionDeleter{ this, id });
}

void DatabasePool::releaseConnection(EDatabaseID id, SQLConnection* connection) const noexcept {
    if (!connection) return;
    auto& state = m_pools[poolIndex(id)];
    try { connection->reset(); } catch (...) { /* a conexao sera reutilizada apos rollback best-effort */ }
    {
        std::lock_guard lock(state.mutex);
        state.available.push_back(connection);
    }
    state.availableCondition.notify_one();
}

void DatabasePool::ConnectionDeleter::operator()(SQLConnection* connection) const noexcept {
    if (pool) pool->releaseConnection(database, connection);
}
