#pragma once
#include "GameLogic/GameSession.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <cmath>
#include <atomic>
#include <algorithm>
#include <optional>
#include "Network/Packet.h"
#include "Network/ClientSession.h"
#include "Shared/datatypes.h"
#include <vector>

class ClientSession;

struct MapPosition { float x = 0.0f; float y = 0.0f; float z = 0.0f; };
struct MapEntity {
    int id = 0;
    int accountId = 0;
    MapPosition position{};
    int mapId = 0;
    int health = 100;
    int maxHealth = 100;
    int attack = 10;
    int defense = 0;
    bool alive = true;
};
struct CombatResult { bool hit = false; bool targetDefeated = false; int damage = 0; int targetHealth = 0; };

class GameWorld {
public:
    std::shared_ptr<GameSession> enter(int accountId, const std::string& accountName, const std::string& characterName, const CharacterData& character, std::weak_ptr<ClientSession> client = {}) {
        auto gameSession = std::make_shared<GameSession>(accountId, accountName, characterName, std::move(client));
        std::lock_guard lock(m_mutex);
        if (auto existing = m_sessions.find(accountName); existing != m_sessions.end())
            m_entities.erase(existing->second->entityId());
        gameSession->setEntityId(m_nextEntityId);
        m_sessions[accountName] = gameSession;
        MapEntity entity{m_nextEntityId, accountId}; entity.position = {character.fPosX, character.fPosY, character.fPosZ}; entity.mapId = character.iMapId; entity.health = character.iHP; entity.maxHealth = character.iMaxHP; entity.attack = std::max(1, character.iAttackMax); entity.defense = character.iDefense; entity.alive = entity.health > 0; m_entities[m_nextEntityId] = entity;
        ++m_nextEntityId;
        return gameSession;
    }
    void leave(const std::string& accountName) {
        std::lock_guard lock(m_mutex);
        auto session = m_sessions.find(accountName);
        if (session != m_sessions.end()) { m_entities.erase(session->second->entityId()); m_sessions.erase(session); }
    }
    bool moveEntity(int entityId, MapPosition destination) {
        if (!std::isfinite(destination.x) || !std::isfinite(destination.y) || !std::isfinite(destination.z)) return false;
        std::lock_guard lock(m_mutex);
        auto it = m_entities.find(entityId); if (it == m_entities.end() || !it->second.alive) return false;
        const auto& p = it->second.position;
        const float dx = destination.x - p.x, dy = destination.y - p.y, dz = destination.z - p.z;
        if (dx * dx + dy * dy + dz * dz > 400.0f) return false;
        it->second.position = destination; return true;
    }
    std::optional<int> entityIdForAccount(const std::string& accountName) const {
        std::lock_guard lock(m_mutex); auto it = m_sessions.find(accountName); return it == m_sessions.end() ? std::nullopt : std::optional<int>(it->second->entityId());
    }
    void broadcastNearby(const std::string& exceptAccount, int sourceEntityId, const Packet& packet, float radius = 120.0f) const {
        std::vector<std::shared_ptr<ClientSession>> clients;
        std::lock_guard lock(m_mutex);
        const auto source = m_entities.find(sourceEntityId);
        if (source == m_entities.end()) return;
        const auto& origin = source->second;
        const auto maxDistance = radius * radius;
        for (const auto& [name, session] : m_sessions) {
            if (name == exceptAccount) continue;
            const auto target = m_entities.find(session->entityId());
            if (target == m_entities.end() || target->second.mapId != origin.mapId) continue;
            const auto dx = target->second.position.x - origin.position.x;
            const auto dy = target->second.position.y - origin.position.y;
            const auto dz = target->second.position.z - origin.position.z;
            if (dx * dx + dy * dy + dz * dz > maxDistance) continue;
            if (auto client = session->client()) clients.push_back(std::move(client));
        }
        for (auto& client : clients) client->send(packet);
    }
    CombatResult attack(int attackerId, int targetId) {
        std::lock_guard lock(m_mutex);
        auto attacker = m_entities.find(attackerId), target = m_entities.find(targetId);
        if (attacker == m_entities.end() || target == m_entities.end() || !attacker->second.alive || !target->second.alive) return {};
        const auto& a = attacker->second.position; const auto& t = target->second.position;
        const float dx = a.x - t.x, dy = a.y - t.y, dz = a.z - t.z;
        if (dx * dx + dy * dy + dz * dz > 36.0f) return {};
        const int damage = std::max(1, attacker->second.attack - target->second.defense);
        target->second.health = std::max(0, target->second.health - damage);
        target->second.alive = target->second.health > 0;
        return {true, !target->second.alive, damage, target->second.health};
    }
    std::optional<MapEntity> entity(int entityId) const {
        std::lock_guard lock(m_mutex); auto it = m_entities.find(entityId); return it == m_entities.end() ? std::nullopt : std::optional<MapEntity>(it->second);
    }
    void update() {
        std::lock_guard lock(m_mutex);
        for (auto& [name, session] : m_sessions) session->update();
    }
    std::size_t size() const { std::lock_guard lock(m_mutex); return m_sessions.size(); }
private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<GameSession>> m_sessions;
    std::unordered_map<int, MapEntity> m_entities;
    int m_nextEntityId = 1;
};
