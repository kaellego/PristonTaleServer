#pragma once

#include "Shared/datatypes.h"
#include "GameLogic/PlayerStateSqlRepository.h"
#include <optional>
#include <string>

class DatabasePool;

class InventorySqlRepository {
public:
    explicit InventorySqlRepository(DatabasePool& pool) : m_state(pool) {}

    std::optional<InventoryData> loadInventory(const std::string& character) const;
    std::optional<WarehouseData> loadWarehouse(const std::string& account) const;
    bool saveInventory(const std::string& character, const InventoryData& data) const;
    bool saveWarehouse(const std::string& account, const WarehouseData& data) const;
    bool saveInventoryAndWarehouse(const std::string& character, const InventoryData& inventory, const std::string& account, const WarehouseData& warehouse) const;

private:
    PlayerStateSqlRepository m_state;
};
