#include "GameLogic/InventorySqlRepository.h"
#include "GameLogic/PlayerStateSqlRepository.h"

std::optional<InventoryData> InventorySqlRepository::loadInventory(const std::string& character) const {
    return m_state.loadInventory(character);
}

std::optional<WarehouseData> InventorySqlRepository::loadWarehouse(const std::string& account) const {
    return m_state.loadWarehouse(account);
}

bool InventorySqlRepository::saveInventory(const std::string& character, const InventoryData& data) const {
    return m_state.saveInventory(character, data);
}

bool InventorySqlRepository::saveWarehouse(const std::string& account, const WarehouseData& data) const {
    return m_state.saveWarehouse(account, data);
}

bool InventorySqlRepository::saveInventoryAndWarehouse(const std::string& character, const InventoryData& inventory, const std::string& account, const WarehouseData& warehouse) const {
    return m_state.saveInventoryAndWarehouse(character, inventory, account, warehouse);
}

bool InventorySqlRepository::saveCharacterAndInventory(const CharacterData& character, const InventoryData& inventory) const {
    return m_state.saveCharacterAndInventory(character, inventory);
}
