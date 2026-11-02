#pragma once

#include "Shared/datatypes.h"

#include <optional>
#include <string>
#include <mutex>

class StorageFileRepository final {
public:
    explicit StorageFileRepository(const std::string& dataPath);

    std::optional<WarehouseData> loadWarehouse(const std::string& accountName) const;
    bool saveWarehouse(const std::string& accountName, const WarehouseData& warehouse) const;
    bool deleteWarehouse(const std::string& accountName) const;

    std::optional<InventoryData> loadInventory(const std::string& characterName) const;
    bool saveInventory(const std::string& characterName, const InventoryData& inventory) const;
    bool deleteInventory(const std::string& characterName) const;

private:
    std::string m_warehousePath;
    std::string m_inventoryPath;
    mutable std::mutex m_mutex;
};
