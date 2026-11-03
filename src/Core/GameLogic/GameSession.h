#pragma once
#include <chrono>
#include <mutex>
#include <string>
#include <memory>
#include "Shared/datatypes.h"

class ClientSession;

class GameSession {
public:
    GameSession(int accountId, std::string accountName, std::string characterName, std::weak_ptr<ClientSession> client = {})
        : m_accountId(accountId), m_accountName(std::move(accountName)), m_characterName(std::move(characterName)), m_client(std::move(client)), m_lastUpdate(std::chrono::steady_clock::now()) {}
    int accountId() const noexcept { return m_accountId; }
    const std::string& accountName() const noexcept { return m_accountName; }
    const std::string& characterName() const noexcept { return m_characterName; }
    void setEntityId(int id) noexcept { m_entityId = id; }
    int entityId() const noexcept { return m_entityId; }
    void setClient(std::weak_ptr<ClientSession> client) { std::lock_guard lock(m_mutex); m_client = std::move(client); }
    void setCharacterData(const CharacterData& data) { std::lock_guard lock(m_mutex); m_characterData = data; }
    CharacterData characterData() const { std::lock_guard lock(m_mutex); return m_characterData; }
    void update() { std::lock_guard lock(m_mutex); m_lastUpdate = std::chrono::steady_clock::now(); }
    std::shared_ptr<ClientSession> client() const { std::lock_guard lock(m_mutex); return m_client.lock(); }
private:
    int m_accountId;
    int m_entityId = 0;
    std::string m_accountName;
    std::string m_characterName;
    std::weak_ptr<ClientSession> m_client;
    CharacterData m_characterData{};
    std::chrono::steady_clock::time_point m_lastUpdate;
    mutable std::mutex m_mutex;
};
