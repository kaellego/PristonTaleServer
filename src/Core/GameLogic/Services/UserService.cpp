#include "GameLogic/Services/UserService.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"

UserService::UserService(LogService& logService) : m_logService(logService) {}

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