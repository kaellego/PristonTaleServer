#include "GameLogic/Services/UserService.h"
#include "Network/ClientSession.h"
#include "Logging/LogService.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include <chrono>

UserService::UserService(DatabasePool &dbPool, LogService &logService) : m_logService(logService), m_dbPool(dbPool) {}

bool UserService::addUser(int accountId, const std::string &accountName, std::shared_ptr<ClientSession> session)
{
    std::shared_ptr<ClientSession> previous;
    {
        std::lock_guard<std::mutex> lock(m_usersMutex);
        if (auto it = m_onlineUsers.find(accountName); it != m_onlineUsers.end())
            previous = it->second.session;

        UserSession userSession;
        userSession.accountId = accountId;
        userSession.accountName = accountName;
        userSession.session = session;
        m_onlineUsers[accountName] = userSession;
    }
    if (previous && previous != session)
        previous->close();

    m_logService.info("Usuario '{}' (ID: {}) adicionado a lista de online. Total: {}", accountName, accountId, m_onlineUsers.size());
    return true;
}

void UserService::removeUser(std::shared_ptr<ClientSession> session)
{
    std::lock_guard<std::mutex> lock(m_usersMutex);
    for (auto it = m_onlineUsers.begin(); it != m_onlineUsers.end(); ++it)
    {
        if (it->second.session == session)
        {
            m_logService.info("Usuario '{}' removido da lista de online. Total: {}", it->first, m_onlineUsers.size() - 1);
            m_onlineUsers.erase(it);
            return;
        }
    }
}

bool UserService::isUserOnline(const std::string &accountName)
{
    std::lock_guard<std::mutex> lock(m_usersMutex);
    return m_onlineUsers.count(accountName) > 0;
}

bool UserService::transferSession(const std::string &accountName, const std::shared_ptr<ClientSession> &session)
{
    std::lock_guard<std::mutex> lock(m_usersMutex);
    auto it = m_onlineUsers.find(accountName);
    if (it == m_onlineUsers.end() || !it->second.session || it->second.session == session)
        return false;

    const auto &source = it->second.session;
    if (!source->isAuthenticated() || source->ticketExpired())
        return false;
    try
    {
        if (source->socket().remote_endpoint().address() != session->socket().remote_endpoint().address())
            return false;
    }
    catch (...)
    {
        return false;
    }

    session->authenticate(source->getAccountId(), accountName, source->getTicket(),
                          std::chrono::steady_clock::now() + std::chrono::minutes(15));
    session->selectCharacter(source->getCharacterName());
    session->setClientObjectSerial(source->clientObjectSerial());
    it->second.session = session;
    return true;
}

bool UserService::transferSessionByIp(const std::shared_ptr<ClientSession> &session)
{
    std::shared_ptr<ClientSession> source;
    std::string accountName;
    int matches = 0;
    std::string remoteIp;
    try
    {
        remoteIp = session->socket().remote_endpoint().address().to_string();
    }
    catch (...)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_usersMutex);
    for (const auto &[name, user] : m_onlineUsers)
    {
        if (!user.session || !user.session->isAuthenticated() || user.session->ticketExpired())
            continue;
        try
        {
            if (user.session->socket().remote_endpoint().address().to_string() != remoteIp)
                continue;
        }
        catch (...)
        {
            continue;
        }
        source = user.session;
        accountName = name;
        ++matches;
    }
    if (matches != 1)
    {
        m_logService.warn("Transferencia GameServer rejeitada para IP '{}': {} sessoes autenticadas encontradas.", remoteIp, matches);
        return false;
    }

    session->authenticate(source->getAccountId(), accountName, source->getTicket(),
                          std::chrono::steady_clock::now() + std::chrono::minutes(15));
    session->selectCharacter(source->getCharacterName());
    session->setClientObjectSerial(source->clientObjectSerial());
    m_onlineUsers[accountName].session = session;
    return true;
}

std::shared_ptr<ClientSession> UserService::sessionForAccount(const std::string &accountName) const
{
    std::lock_guard<std::mutex> lock(m_usersMutex);
    const auto it = m_onlineUsers.find(accountName);
    return it == m_onlineUsers.end() ? nullptr : it->second.session;
}

bool UserService::banAccount(const std::string &accountName, const std::string &reason)
{
    if (accountName.empty())
        return false;
    try
    {
        auto db = m_dbPool.createConnection(EDatabaseID::UserDB_Primary);
        db->prepare("UPDATE UserInfo SET BanStatus=1 WHERE AccountName=?");
        db->bindParameter(1, accountName);
        db->execute();
    }
    catch (const std::exception &)
    {
        return false;
    }
    std::shared_ptr<ClientSession> session;
    {
        std::lock_guard<std::mutex> lock(m_usersMutex);
        auto it = m_onlineUsers.find(accountName);
        if (it != m_onlineUsers.end())
        {
            session = it->second.session;
            m_onlineUsers.erase(it);
        }
    }
    if (session)
        session->close();
    m_logService.warn("Conta '{}' banida automaticamente: {}", accountName, reason);
    return true;
}

bool UserService::isGameMaster(const std::string &accountName) const
{
    if (accountName.empty())
        return false;
    try
    {
        auto db = m_dbPool.createConnection(EDatabaseID::GameDB);
        db->prepare("SELECT TOP 1 Type FROM dbo.GameMasters WHERE AccountID=? AND active=1");
        db->bindParameter(1, accountName);
        db->execute();
        return db->fetch() && db->getData<int>(1).value_or(0) > 0;
    }
    catch (...)
    {
        return false;
    }
}
