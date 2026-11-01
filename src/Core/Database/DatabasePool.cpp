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
    const auto& db = config.getDatabaseConfig();
    m_driver = db.driver; m_host = db.host; m_user = db.user; m_password = db.password;
    m_dialect = detectDatabaseDialect(m_driver);
    if (m_dialect == DatabaseDialect::Unknown) throw std::runtime_error("Driver ODBC nao suportado: " + m_driver);
    m_logService.info("ODBC configurado para os bancos; conexoes sob demanda com SQL_DRIVER_NOPROMPT; workers logicos: {}.", poolSizePerDb);
}

std::string DatabasePool::connectionString(EDatabaseID id) const {
    const auto* name = databaseName(id); if (!name) return {};
    auto driver = m_driver;
    if (driver.size() > 1 && driver.front() == '{' && driver.back() == '}') driver = driver.substr(1, driver.size() - 2);
    m_logService.debug("SOCI ODBC driver='{}' host='{}' database='{}'", driver, m_host, name);
    return std::format("DRIVER={{{}}};SERVER={},1433;DATABASE={};UID={};PWD={};TrustServerCertificate=yes;", driver, m_host, name, m_user, m_password);
}

std::unique_ptr<SQLConnection> DatabasePool::createConnection(EDatabaseID id) const {
    return std::make_unique<SQLConnection>(connectionString(id));
}
