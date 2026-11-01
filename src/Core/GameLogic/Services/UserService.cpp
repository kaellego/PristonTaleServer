#include "GameLogic/Services/UserService.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"

UserService::UserService(DatabasePool& dbPool, LogService& logService) : m_logService(logService), m_dbPool(dbPool) {}

bool UserService::addUser(int accountId, const std::string& accountName, std::shared_ptr<ClientSession> session) {
    std::shared_ptr<ClientSession> previous;
    {
        std::lock_guard<std::mutex> lock(m_usersMutex);
        if (auto it = m_onlineUsers.find(accountName); it != m_onlineUsers.end()) previous = it->second.session;

        UserSession userSession;
        userSession.accountId = accountId;
        userSession.accountName = accountName;
        userSession.session = session;
        m_onlineUsers[accountName] = userSession;
    }
    if (previous && previous != session) previous->close();

    m_logService.info("Usuario '{}' (ID: {}) adicionado a lista de online. Total: {}", accountName, accountId, m_onlineUsers.size());
    return true;
}

void UserService::removeUser(std::shared_ptr<ClientSession> session) {
    std::lock_guard<std::mutex> lock(m_usersMutex);
    for (auto it = m_onlineUsers.begin(); it != m_onlineUsers.end(); ++it) {
        if (it->second.session == session) {
            m_logService.info("Usuario '{}' removido da lista de online. Total: {}", it->first, m_onlineUsers.size() - 1);
            m_onlineUsers.erase(it);
            return;
        }
    }
}

bool UserService::isUserOnline(const std::string& accountName) {
    std::lock_guard<std::mutex> lock(m_usersMutex);
    return m_onlineUsers.count(accountName) > 0;
}

bool UserService::banAccount(const std::string& accountName, const std::string& reason) {
    if (accountName.empty()) return false;
    try {
        auto db = m_dbPool.createConnection(EDatabaseID::UserDB_Primary); db->prepare("UPDATE UserInfo SET BanStatus=1 WHERE AccountName=?"); db->bindParameter(1, accountName); db->execute();
    } catch (const std::exception&) {
        return false;
    }
    std::shared_ptr<ClientSession> session;
    { std::lock_guard<std::mutex> lock(m_usersMutex); auto it = m_onlineUsers.find(accountName); if (it != m_onlineUsers.end()) { session = it->second.session; m_onlineUsers.erase(it); } }
    if (session) session->close();
    m_logService.warn("Conta '{}' banida automaticamente: {}", accountName, reason);
    return true;
}
