#include "GameLogic/Services/UserService.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"

UserService::UserService(DatabasePool& dbPool, LogService& logService) : m_logService(logService), m_dbPool(dbPool) {}

void UserService::addUser(int accountId, const std::string& accountName, std::shared_ptr<ClientSession> session) {
    std::lock_guard<std::mutex> lock(m_usersMutex);

    UserSession userSession;
    userSession.accountId = accountId;
    userSession.accountName = accountName;
    userSession.session = session;

    m_onlineUsers[accountName] = userSession;
    m_logService.info("Usuario '{}' (ID: {}) adicionado a lista de online. Total: {}", accountName, accountId, m_onlineUsers.size());
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
    auto db = m_dbPool.getConnection(EDatabaseID::UserDB_Primary);
    if (!db) return false;
    db->prepare("UPDATE UserInfo SET BanStatus=1 WHERE AccountName=?");
    db->bindParameter<std::string>(1, accountName);
    db->execute();
    std::shared_ptr<ClientSession> session;
    { std::lock_guard<std::mutex> lock(m_usersMutex); auto it = m_onlineUsers.find(accountName); if (it != m_onlineUsers.end()) { session = it->second.session; m_onlineUsers.erase(it); } }
    if (session) session->close();
    m_logService.warn("Conta '{}' banida automaticamente: {}", accountName, reason);
    return true;
}
