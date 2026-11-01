#include "GameLogic/Services/InventoryService.h"
#include "GameLogic/PlayerRepository.h"
#include "GameLogic/Services/CharacterService.h"
#include "GameLogic/ItemPolicy.h"
#include <utility>

InventoryService::InventoryService(PlayerRepository& players, CharacterService& characters, ItemPolicy& itemPolicy)
    : m_players(players), m_characters(characters), m_itemPolicy(itemPolicy), m_worker(&InventoryService::workerLoop, this) {}

InventoryService::~InventoryService() {
    { std::lock_guard lock(m_mutex); m_stopping = true; }
    m_condition.notify_one();
    if (m_worker.joinable()) m_worker.join();
}

void InventoryService::enqueue(std::function<void()> task) {
    { std::lock_guard lock(m_mutex); if (m_stopping) return; m_tasks.push(std::move(task)); }
    m_condition.notify_one();
}

void InventoryService::workerLoop() {
    while (true) {
        std::function<void()> task;
        { std::unique_lock lock(m_mutex); m_condition.wait(lock, [this] { return m_stopping || !m_tasks.empty(); });
          if (m_stopping && m_tasks.empty()) return; task = std::move(m_tasks.front()); m_tasks.pop(); }
        task();
    }
}

void InventoryService::loadInventory(const std::string& account, const std::string& character, InventoryCallback callback) {
    enqueue([this, account, character, callback = std::move(callback)]() mutable {
        if (!m_characters.ownsCharacter(account, character)) { callback(false, {}); return; }
        auto data = m_players.loadInventory(character).value_or(InventoryData{});
        if (!m_players.saveInventory(character, data)) { callback(false, {}); return; }
        callback(true, data);
    });
}

void InventoryService::loadWarehouse(const std::string& account, const std::string& character, WarehouseCallback callback) {
    enqueue([this, account, character, callback = std::move(callback)]() mutable {
        if (!m_characters.ownsCharacter(account, character)) { callback(false, {}); return; }
        auto data = m_players.loadWarehouse(account).value_or(WarehouseData{});
        if (!m_players.saveWarehouse(account, data)) { callback(false, {}); return; }
        callback(true, data);
    });
}

void InventoryService::moveInventoryItem(const std::string& account, const std::string& character, int source, int target, InventoryCallback callback) {
    enqueue([this, account, character, source, target, callback = std::move(callback)]() mutable {
        if (source < 0 || source >= static_cast<int>(PLAYER_INVENTORY_SLOTS) || target < 0 || target >= static_cast<int>(PLAYER_INVENTORY_SLOTS) || !m_characters.ownsCharacter(account, character)) { callback(false, {}); return; }
        auto data = m_players.loadInventory(character).value_or(InventoryData{});
        std::swap(data.saItem[source], data.saItem[target]);
        if (!m_players.saveInventory(character, data)) { callback(false, {}); return; }
        callback(true, data);
    });
}

void InventoryService::useOrEquipItem(const std::string& account, const std::string& character, int inventorySlot, int equipmentSlot, int action, InventoryCallback callback) {
    enqueue([this, account, character, inventorySlot, equipmentSlot, action, callback = std::move(callback)]() mutable {
        if (inventorySlot < 0 || inventorySlot >= static_cast<int>(PLAYER_INVENTORY_SLOTS) || equipmentSlot < 0 || equipmentSlot >= 16 || (action != 1 && action != 2) || !m_characters.ownsCharacter(account, character)) { callback(false, {}); return; }
        auto inventory = m_players.loadInventory(character).value_or(InventoryData{});
        auto characterData = m_players.loadCharacter(character);
        if (!characterData || inventory.saItem[inventorySlot].sItemID <= 0) { callback(false, {}); return; }
        const auto itemCode = inventory.saItem[inventorySlot].sItemID;
        if (action == 1 && !m_itemPolicy.canEquip(itemCode, characterData->iLevel, characterData->iClass, equipmentSlot)) { callback(false, {}); return; }
        if (action == 2 && !m_itemPolicy.canUse(itemCode)) { callback(false, {}); return; }
        if (action == 1) std::swap(inventory.saItem[inventorySlot], characterData->saEquipment[equipmentSlot]);
        else { characterData->iHP = std::min(characterData->iMaxHP, characterData->iHP + 10); inventory.saItem[inventorySlot] = {}; }
        if (!m_players.saveCharacter(*characterData) || !m_players.saveInventory(character, inventory)) { callback(false, {}); return; }
        callback(true, inventory);
    });
}
