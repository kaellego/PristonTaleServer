#pragma once

#include "Shared/datatypes.h"

#include <optional>
#include <string>
#include <vector>

class DatabasePool;
class SQLConnection;

class PlayerStateSqlRepository final {
public:
    explicit PlayerStateSqlRepository(DatabasePool& pool) : m_pool(pool) {}

    std::optional<CharacterData> loadCharacter(const std::string& name) const;
    bool hasCharacterState(const std::string& name) const;
    bool saveCharacter(const CharacterData& value) const;
    bool deleteCharacter(const std::string& name) const;

    std::optional<InventoryData> loadInventory(const std::string& character) const;
    bool hasInventoryState(const std::string& character) const;
    bool saveInventory(const std::string& character, const InventoryData& value) const;
    bool deleteInventory(const std::string& character) const;

    std::optional<WarehouseData> loadWarehouse(const std::string& account) const;
    bool hasWarehouseState(const std::string& account) const;
    bool saveWarehouse(const std::string& account, const WarehouseData& value) const;
    bool deleteWarehouse(const std::string& account) const;
    bool saveInventoryAndWarehouse(const std::string& character, const InventoryData& inventory,
                                   const std::string& account, const WarehouseData& warehouse) const;
    bool saveCharacterAndInventory(const CharacterData& character, const InventoryData& inventory) const;

private:
    struct LoadedBlob {
        std::vector<uint8_t> bytes;
        int version = 1;
    };

    std::optional<LoadedBlob> loadBlob(int type, const std::string& owner) const;
    bool hasBlob(int type, const std::string& owner) const;
    bool saveBlob(SQLConnection& db, int type, const std::string& owner, const std::vector<uint8_t>& bytes) const;
    bool deleteBlob(int type, const std::string& owner) const;

    DatabasePool& m_pool;
};
