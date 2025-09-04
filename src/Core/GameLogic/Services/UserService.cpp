#include "GameLogic/Services/UserService.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"

UserService::UserService(LogService& logService) : m_logService(logService) {}

void UserService::addUser(int accountId, std::shared_ptr<ClientSession> session) {
    // std::lock_guard garante que o mutex seja liberado automaticamente
    std::lock_guard<std::mutex> lock(m_usersMutex);

    UserSession userSession;
    userSession.accountId = accountId;
    // userSession.accountName = ...; // O nome da conta viria após a autenticação
    userSession.session = session;

    m_onlineUsers[accountId] = userSession;
    m_logService.info("Usuario com ID de conta {} adicionado a lista de online. Total: {}", accountId, m_onlineUsers.size());
}

void UserService::removeUser(std::shared_ptr<ClientSession> session) {
    std::lock_guard<std::mutex> lock(m_usersMutex);

    // Itera sobre o mapa para encontrar o usuário com a sessão correspondente e removê-lo
    for (auto it = m_onlineUsers.begin(); it != m_onlineUsers.end(); ++it) {
        if (it->second.session == session) {
            m_logService.info("Usuario com ID de conta {} removido da lista de online. Total: {}", it->first, m_onlineUsers.size() - 1);
            m_onlineUsers.erase(it);
            return;
        }
    }
}

bool UserService::isUserOnline(int accountId) {
    std::lock_guard<std::mutex> lock(m_usersMutex);
    return m_onlineUsers.count(accountId) > 0;
}