#pragma once
#include "Database/DatabaseDialect.h"
#include "Database/SQLConnection.h"
#include <string>
#include <cstddef>
#include <memory>
#include <array>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

class ServerConfig;
class LogService;
enum class EDatabaseID { GameDB, LogDB, UserDB_Primary, Invalid };

class DatabasePool {
public:
    struct ConnectionDeleter {
        const DatabasePool* pool = nullptr;
        EDatabaseID database = EDatabaseID::Invalid;
        void operator()(SQLConnection* connection) const noexcept;
    };
    using Connection = std::unique_ptr<SQLConnection, ConnectionDeleter>;

    explicit DatabasePool(const ServerConfig& config, LogService& logService, size_t poolSizePerDb = 5);
    DatabasePool(const DatabasePool&) = delete;
    DatabasePool& operator=(const DatabasePool&) = delete;
    DatabaseDialect dialect() const noexcept { return m_dialect; }
    std::string connectionString(EDatabaseID id) const;
    Connection createConnection(EDatabaseID id) const;
private:
    struct PoolState {
        mutable std::mutex mutex;
        mutable std::condition_variable availableCondition;
        mutable std::deque<SQLConnection*> available;
        std::vector<std::unique_ptr<SQLConnection>> owned;
    };

    static std::size_t poolIndex(EDatabaseID id);
    void releaseConnection(EDatabaseID id, SQLConnection* connection) const noexcept;

    LogService& m_logService;
    DatabaseDialect m_dialect = DatabaseDialect::Unknown;
    std::string m_driver, m_host, m_user, m_password;
    std::array<PoolState, 3> m_pools;
};
