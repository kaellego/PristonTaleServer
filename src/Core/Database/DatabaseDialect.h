#pragma once

#include <string>
#include <algorithm>

enum class DatabaseDialect { SqlServer, PostgreSQL, MySQL, Unknown };

inline DatabaseDialect detectDatabaseDialect(std::string driver) {
    std::transform(driver.begin(), driver.end(), driver.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (driver.find("sql server") != std::string::npos || driver.find("sqlsrv") != std::string::npos) return DatabaseDialect::SqlServer;
    if (driver.find("postgres") != std::string::npos) return DatabaseDialect::PostgreSQL;
    if (driver.find("mysql") != std::string::npos || driver.find("mariadb") != std::string::npos) return DatabaseDialect::MySQL;
    return DatabaseDialect::Unknown;
}

inline std::string selectLimit(DatabaseDialect dialect, int limit) {
    return dialect == DatabaseDialect::SqlServer
        ? " TOP " + std::to_string(limit)
        : "";
}

inline std::string queryLimit(DatabaseDialect dialect, int limit) {
    return dialect == DatabaseDialect::SqlServer
        ? ""
        : " LIMIT " + std::to_string(limit);
}
