#pragma once
#include <chrono>
#include <mutex>
#include <string>

class GameSession {
public:
    GameSession(int accountId, std::string accountName, std::string characterName)
        : m_accountId(accountId), m_accountName(std::move(accountName)), m_characterName(std::move(characterName)), m_lastUpdate(std::chrono::steady_clock::now()) {}
    int accountId() const noexcept { return m_accountId; }
    const std::string& accountName() const noexcept { return m_accountName; }
    const std::string& characterName() const noexcept { return m_characterName; }
    void setEntityId(int id) noexcept { m_entityId = id; }
    int entityId() const noexcept { return m_entityId; }
    void update() { std::lock_guard lock(m_mutex); m_lastUpdate = std::chrono::steady_clock::now(); }
private:
    int m_accountId;
    int m_entityId = 0;
    std::string m_accountName;
    std::string m_characterName;
    std::chrono::steady_clock::time_point m_lastUpdate;
    mutable std::mutex m_mutex;
};
