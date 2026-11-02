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
#include "Shared/Constants.h"
#include <vector>
#include <chrono>
#include <functional>
#include <cstring>

class ClientSession;

struct MapPosition { float x = 0.0f; float y = 0.0f; float z = 0.0f; };
struct MapEntity {
    int id = 0;
    int accountId = 0;
    uint32_t objectSerial = 0;
    MapPosition position{};
    int mapId = 0;
    int health = 100;
    int maxHealth = 100;
    int mana = 100;
    int maxMana = 100;
    int stamina = 100;
    int maxStamina = 100;
    float hpRegen = 0.0f;
    float mpRegen = 0.0f;
    float spRegen = 0.0f;
    float hpRemainder = 0.0f;
    float mpRemainder = 0.0f;
    float spRemainder = 0.0f;
    std::chrono::steady_clock::time_point nextRegen{};
    int attack = 10;
    int defense = 0;
    bool alive = true;
};
struct CombatResult { bool hit = false; bool targetDefeated = false; int damage = 0; int targetHealth = 0; };
struct WorldDrop { int id = 0; int mapId = 0; MapPosition position{}; Item item{}; std::chrono::steady_clock::time_point expiresAt{}; };
struct NpcEntity {
    int id = 0;
    uint32_t objectSerial = 0;
    int mapId = 0;
    MapPosition position{};
    int angleY = 0;
    std::string name;
    std::string modelName;
    int eventCode = 0;
    int health = 100;
    int maxHealth = 100;
    int dropItemCode = 0;
    int dropQuantity = 0;
    bool inCombat = false;
    bool enabled = true;
};

class GameWorld {
public:
    using PersistenceCallback = std::function<void(const std::string&, CharacterData)>;
    void setPersistenceCallback(PersistenceCallback callback) { m_persist = std::move(callback); }
    bool registerNpc(NpcEntity npc) {
        if (npc.id <= 0 || npc.objectSerial == 0 || npc.name.empty() || npc.modelName.empty() ||
            npc.name.size() >= 32 || npc.modelName.size() >= 64 || !std::isfinite(npc.position.x) ||
            !std::isfinite(npc.position.y) || !std::isfinite(npc.position.z) || npc.maxHealth <= 0 ||
            npc.health < 0 || npc.health > npc.maxHealth || npc.dropQuantity < 0) return false;
        std::lock_guard lock(m_mutex);
        if (m_npcs.size() >= 512 || m_npcs.contains(npc.objectSerial)) return false;
        m_npcs.emplace(npc.objectSerial, std::move(npc));
        return true;
    }
    std::vector<NpcEntity> npcsForMap(int mapId) const {
        std::vector<NpcEntity> result;
        std::lock_guard lock(m_mutex);
        for (const auto& [serial, npc] : m_npcs)
            if (npc.enabled && npc.mapId == mapId) result.push_back(npc);
        return result;
    }
    std::optional<NpcEntity> interactNpc(int entityId, int npcId, uint32_t objectSerial) const {
        std::lock_guard lock(m_mutex);
        const auto player = m_entities.find(entityId);
        if (player == m_entities.end() || !player->second.alive || objectSerial == 0) return std::nullopt;
        const NpcEntity* npc = nullptr;
        if (npcId > 0) {
            for (const auto& [serial, candidate] : m_npcs)
                if (candidate.id == npcId && candidate.objectSerial == objectSerial) { npc = &candidate; break; }
        } else {
            for (const auto& [serial, candidate] : m_npcs)
                if (candidate.objectSerial == objectSerial) { npc = &candidate; break; }
        }
        if (!npc || !npc->enabled || npc->objectSerial != objectSerial || npc->mapId != player->second.mapId)
            return std::nullopt;
        const auto& p = player->second.position;
        // The legacy PLAYERINFO request carries only a serial. Its cached
        // integer coordinates may be stale; map and serial validation above
        // remain authoritative.
        if (npcId == 0) return *npc;
        const float dx = p.x - npc->position.x, dy = p.y - npc->position.y, dz = p.z - npc->position.z;
        // O cliente legado envia coordenadas inteiras; o alcance visual do NPC
        // é maior que a precisão desse pacote e não deve rejeitar cliques válidos.
        constexpr float npcInteractionRadius = 36.0f * 256.0f;
        if (dx * dx + dy * dy + dz * dz > npcInteractionRadius * npcInteractionRadius) return std::nullopt;
        return *npc;
    }
    bool damageNpc(uint32_t objectSerial, int damage, int& remainingHealth) {
        if (damage <= 0) return false;
        std::lock_guard lock(m_mutex);
        auto it = m_npcs.find(objectSerial);
        if (it == m_npcs.end() || !it->second.enabled || it->second.health <= 0) return false;
        it->second.inCombat = true;
        it->second.health = std::max(0, it->second.health - damage);
        remainingHealth = it->second.health;
        if (remainingHealth == 0) it->second.inCombat = false;
        return true;
    }
    std::optional<Item> npcDrop(uint32_t objectSerial) const {
        std::lock_guard lock(m_mutex);
        const auto it = m_npcs.find(objectSerial);
        if (it == m_npcs.end() || it->second.health != 0 || it->second.dropItemCode <= 0 || it->second.dropQuantity <= 0)
            return std::nullopt;
        Item drop{};
        drop.sItemID = it->second.dropItemCode;
        drop.iQuantity = it->second.dropQuantity;
        return drop;
    }
    std::shared_ptr<GameSession> enter(int accountId, const std::string& accountName, const std::string& characterName, const CharacterData& character, std::weak_ptr<ClientSession> client = {}) {
        auto gameSession = std::make_shared<GameSession>(accountId, accountName, characterName, std::move(client));
        gameSession->setCharacterData(character);
        std::lock_guard lock(m_mutex);
        if (auto existing = m_sessions.find(accountName); existing != m_sessions.end())
            m_entities.erase(existing->second->entityId());
        gameSession->setEntityId(m_nextEntityId);
        m_sessions[accountName] = gameSession;
        MapEntity entity{m_nextEntityId, accountId};
        entity.objectSerial = 0x10000000u + static_cast<uint32_t>(std::hash<std::string>{}(characterName));
        entity.position = {character.fPosX, character.fPosY, character.fPosZ};
        entity.mapId = character.iMapId;
        entity.maxHealth = std::max(100, character.iMaxHP);
        entity.health = character.iHP > 0 ? std::clamp(character.iHP, 1, entity.maxHealth) : entity.maxHealth;
        entity.maxMana = std::max(100, character.iMaxMP);
        entity.mana = character.iMP > 0 ? std::clamp(character.iMP, 1, entity.maxMana) : entity.maxMana;
        entity.maxStamina = std::max(100, character.iMaxSTM);
        entity.stamina = character.iSTM > 0 ? std::clamp(character.iSTM, 1, entity.maxStamina) : entity.maxStamina;
        entity.hpRegen = character.fHPRegen + character.fHPRegenBonus;
        entity.mpRegen = character.fMPRegen + character.fMPRegenBonus;
        entity.spRegen = character.fSPRegen + character.fSPRegenBonus;
        for (const auto& item : character.saEquipment) { entity.hpRegen += item.fHPRegen; entity.mpRegen += item.fMPRegen; entity.spRegen += item.fSPRegen; }
        entity.nextRegen = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        entity.attack = std::max(1, character.iAttackMax);
        entity.defense = character.iDefense;
        entity.alive = entity.health > 0;
        m_entities[m_nextEntityId] = entity;
        ++m_nextEntityId;
        return gameSession;
    }
    void leave(const std::string& accountName) {
        std::optional<CharacterData> snapshot;
        {
            std::lock_guard lock(m_mutex);
            auto session = m_sessions.find(accountName);
            if (session == m_sessions.end()) return;
            const auto entity = m_entities.find(session->second->entityId());
            if (entity != m_entities.end()) {
                CharacterData data = session->second->characterData();
                applyRuntimeState(data, entity->second);
                snapshot = data;
                m_entities.erase(entity);
            }
            m_sessions.erase(session);
        }
        if (snapshot && m_persist) m_persist(std::string(snapshot->szName), *snapshot);
    }
    bool moveEntity(int entityId, MapPosition destination) {
        if (!std::isfinite(destination.x) || !std::isfinite(destination.y) || !std::isfinite(destination.z)) return false;
        std::optional<CharacterData> snapshot;
        {
            std::lock_guard lock(m_mutex);
            auto it = m_entities.find(entityId); if (it == m_entities.end() || !it->second.alive) return false;
            const auto& p = it->second.position;
            const float dx = destination.x - p.x, dy = destination.y - p.y, dz = destination.z - p.z;
            // Legacy coordinates use the client's fixed-point world scale. A
            // 20-unit limit rejected normal movement and left the old spawn
            // location in persistence.
            constexpr float maxMovementStep = 4096.0f;
            if (dx * dx + dy * dy + dz * dz > maxMovementStep * maxMovementStep) return false;
            it->second.position = destination;
            for (const auto& [name, session] : m_sessions) {
                if (session->entityId() != entityId) continue;
                CharacterData data = session->characterData();
                applyRuntimeState(data, it->second);
                session->setCharacterData(data);
                snapshot = data;
                break;
            }
        }
        if (snapshot && m_persist) m_persist(std::string(snapshot->szName), *snapshot);
        return true;
    }
    std::optional<int> entityIdForAccount(const std::string& accountName) const {
        std::lock_guard lock(m_mutex); auto it = m_sessions.find(accountName); return it == m_sessions.end() ? std::nullopt : std::optional<int>(it->second->entityId());
    }
    bool ownsObjectSerial(int entityId, uint32_t objectSerial) const {
        std::lock_guard lock(m_mutex);
        auto it = m_entities.find(entityId);
        return it != m_entities.end() && it->second.objectSerial == objectSerial;
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
    std::optional<int> spawnDrop(int entityId, Item item) {
        std::lock_guard lock(m_mutex);
        auto entityIt = m_entities.find(entityId);
        if (entityIt == m_entities.end() || !entityIt->second.alive || item.sItemID <= 0 || item.iQuantity <= 0 || m_drops.size() >= 4096) return std::nullopt;
        WorldDrop drop{}; drop.id = m_nextDropId++; drop.mapId = entityIt->second.mapId; drop.position = entityIt->second.position; drop.item = item;
        drop.expiresAt = std::chrono::steady_clock::now() + std::chrono::minutes(5);
        m_drops.emplace(drop.id, drop); return drop.id;
    }
    std::optional<Item> takeDrop(int entityId, int dropId) {
        std::lock_guard lock(m_mutex);
        auto entityIt = m_entities.find(entityId); auto dropIt = m_drops.find(dropId);
        if (entityIt == m_entities.end() || dropIt == m_drops.end()) return std::nullopt;
        const auto& e = entityIt->second; const auto& d = dropIt->second;
        const float dx = e.position.x - d.position.x, dy = e.position.y - d.position.y, dz = e.position.z - d.position.z;
        if (e.mapId != d.mapId || dx * dx + dy * dy + dz * dz > 36.0f || d.expiresAt <= std::chrono::steady_clock::now()) return std::nullopt;
        Item item = d.item; m_drops.erase(dropIt); return item;
    }
    void restoreDrop(Item item, int entityId) { spawnDrop(entityId, item); }
    void update() {
        std::vector<std::pair<std::string, CharacterData>> saves;
        std::vector<std::pair<std::shared_ptr<ClientSession>, int>> notifications;
        const auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard lock(m_mutex);
            for (auto& [name, session] : m_sessions) session->update();
            for (auto& [name, session] : m_sessions) {
                auto it = m_entities.find(session->entityId());
                if (it == m_entities.end() || !it->second.alive || now < it->second.nextRegen) continue;
                auto& e = it->second; e.nextRegen = now + std::chrono::seconds(1);
                const int oldHealth = e.health;
                const int oldMana = e.mana;
                const int oldStamina = e.stamina;
                e.hpRemainder += std::max(0.0f, e.hpRegen); e.mpRemainder += std::max(0.0f, e.mpRegen); e.spRemainder += std::max(0.0f, e.spRegen);
                e.health = std::min(e.maxHealth, e.health + static_cast<int>(e.hpRemainder)); e.mana = std::min(e.maxMana, e.mana + static_cast<int>(e.mpRemainder)); e.stamina = std::min(e.maxStamina, e.stamina + static_cast<int>(e.spRemainder));
                e.hpRemainder -= static_cast<int>(e.hpRemainder); e.mpRemainder -= static_cast<int>(e.mpRemainder); e.spRemainder -= static_cast<int>(e.spRemainder);
                const bool changed = e.health != oldHealth || e.mana != oldMana || e.stamina != oldStamina;
                if (changed) {
                    if (auto client = session->client()) notifications.emplace_back(std::move(client), e.health - oldHealth);
                    CharacterData saved = session->characterData();
                    applyRuntimeState(saved, e);
                    session->setCharacterData(saved);
                    saves.emplace_back(session->characterName(), std::move(saved));
                }
            }
            for (auto it = m_drops.begin(); it != m_drops.end();) it = it->second.expiresAt <= now ? m_drops.erase(it) : std::next(it);
        }
        for (auto& [client, amount] : notifications) { std::vector<uint8_t> body(sizeof(int32_t) * 4); std::memcpy(body.data() + sizeof(int32_t), &amount, sizeof(amount)); client->send(Packet(static_cast<uint32_t>(Opcodes::RegenChar), std::move(body))); }
        if (m_persist) for (auto& [name, data] : saves) m_persist(name, std::move(data));
    }
    std::size_t size() const { std::lock_guard lock(m_mutex); return m_sessions.size(); }
private:
    static void applyRuntimeState(CharacterData& data, const MapEntity& entity) {
        data.iMapId = entity.mapId;
        data.fPosX = entity.position.x;
        data.fPosY = entity.position.y;
        data.fPosZ = entity.position.z;
        data.iHP = std::clamp(entity.health, 0, entity.maxHealth);
        data.iMaxHP = entity.maxHealth;
        data.iMP = std::clamp(entity.mana, 0, entity.maxMana);
        data.iMaxMP = entity.maxMana;
        data.iSTM = std::clamp(entity.stamina, 0, entity.maxStamina);
        data.iMaxSTM = entity.maxStamina;
    }

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<GameSession>> m_sessions;
    std::unordered_map<int, MapEntity> m_entities;
    std::unordered_map<uint32_t, NpcEntity> m_npcs;
    int m_nextEntityId = 1;
    int m_nextDropId = 1;
    std::unordered_map<int, WorldDrop> m_drops;
    PersistenceCallback m_persist;
};
