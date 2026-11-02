#include "GameLogic/PlayerRepository.h"

#include "Database/DatabasePool.h"

#include <iostream>

PlayerRepository::PlayerRepository(const std::string& dataPath, DatabasePool* dbPool)
    : m_characters(dataPath), m_storage(dataPath), m_sql(dbPool ? std::make_unique<PlayerStateSqlRepository>(*dbPool) : nullptr) {
    if (!m_sql) return;
    for (const auto& name : m_characters.names()) {
        if (m_sql->hasCharacterState(name)) continue;
        if (auto legacy = m_characters.load(name); legacy && m_sql->saveCharacter(*legacy))
            std::cout << "Estado de personagem migrado para SQL/Protobuf: " << name << std::endl;
    }
}

std::optional<CharacterData> PlayerRepository::loadCharacter(const std::string& name) const {
    if (m_sql) {
        if (auto value = m_sql->loadCharacter(name)) return value;
        if (m_sql->hasCharacterState(name)) return std::nullopt;
        if (auto legacy = m_characters.load(name)) {
            m_sql->saveCharacter(*legacy);
            return legacy;
        }
        return std::nullopt;
    }
    return m_characters.load(name);
}

bool PlayerRepository::characterFileExists(const std::string& name) const {
    return (m_sql && m_sql->hasCharacterState(name)) || m_characters.exists(name);
}

bool PlayerRepository::saveCharacter(const CharacterData& character) const {
    return m_sql ? m_sql->saveCharacter(character) : m_characters.save(character);
}

bool PlayerRepository::deleteCharacter(const std::string& name) const {
    const bool sqlDeleted = !m_sql || m_sql->deleteCharacter(name);
    return sqlDeleted && (!m_characters.exists(name) || m_characters.remove(name));
}

std::optional<WarehouseData> PlayerRepository::loadWarehouse(const std::string& account) const {
    if (m_sql) {
        if (auto value = m_sql->loadWarehouse(account)) return value;
        if (m_sql->hasWarehouseState(account)) return std::nullopt;
        if (auto legacy = m_storage.loadWarehouse(account)) { m_sql->saveWarehouse(account, *legacy); return legacy; }
        return std::nullopt;
    }
    return m_storage.loadWarehouse(account);
}

bool PlayerRepository::saveWarehouse(const std::string& account, const WarehouseData& warehouse) const {
    return m_sql ? m_sql->saveWarehouse(account, warehouse) : m_storage.saveWarehouse(account, warehouse);
}

bool PlayerRepository::deleteWarehouse(const std::string& account) const {
    const bool sqlDeleted = !m_sql || m_sql->deleteWarehouse(account);
    return sqlDeleted && m_storage.deleteWarehouse(account);
}

std::optional<InventoryData> PlayerRepository::loadInventory(const std::string& character) const {
    if (m_sql) {
        if (auto value = m_sql->loadInventory(character)) return value;
        if (m_sql->hasInventoryState(character)) return std::nullopt;
        if (auto legacy = m_storage.loadInventory(character)) { m_sql->saveInventory(character, *legacy); return legacy; }
        return std::nullopt;
    }
    return m_storage.loadInventory(character);
}

bool PlayerRepository::saveInventory(const std::string& character, const InventoryData& inventory) const {
    return m_sql ? m_sql->saveInventory(character, inventory) : m_storage.saveInventory(character, inventory);
}

bool PlayerRepository::deleteInventory(const std::string& character) const {
    const bool sqlDeleted = !m_sql || m_sql->deleteInventory(character);
    return sqlDeleted && m_storage.deleteInventory(character);
}

std::vector<std::string> PlayerRepository::getAllCharacterNames() const {
    return m_characters.names();
}
