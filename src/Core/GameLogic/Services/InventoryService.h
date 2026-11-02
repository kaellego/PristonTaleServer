#pragma once

#include "Shared/datatypes.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <optional>
#include <thread>

class CharacterService;
class PlayerRepository;
class ItemPolicy;
class InventorySqlRepository;

class InventoryService {
public:
    using InventoryCallback = std::function<void(bool, InventoryData)>;
    using WarehouseCallback = std::function<void(bool, WarehouseData)>;
    using TransferCallback = std::function<void(bool, InventoryData, WarehouseData)>;
    using ItemCallback = std::function<void(bool, InventoryData, Item)>;

    InventoryService(PlayerRepository& players, CharacterService& characters, ItemPolicy& itemPolicy, InventorySqlRepository* sqlRepository = nullptr);
    ~InventoryService();
    InventoryService(const InventoryService&) = delete;

    void loadInventory(const std::string& account, const std::string& character, InventoryCallback callback);
    void loadWarehouse(const std::string& account, const std::string& character, WarehouseCallback callback);
    void moveInventoryItem(const std::string& account, const std::string& character, int source, int target, InventoryCallback callback);
    void useOrEquipItem(const std::string& account, const std::string& character, int inventorySlot, int equipmentSlot, int action, InventoryCallback callback);
    void transferItem(const std::string& account, const std::string& character, int inventorySlot, int warehouseSlot, int quantity, bool toWarehouse, TransferCallback callback);
    void dropItem(const std::string& account, const std::string& character, int inventorySlot, int quantity, ItemCallback callback);
    void addItem(const std::string& account, const std::string& character, Item item, InventoryCallback callback);

private:
    std::optional<InventoryData> loadInventoryData(const std::string& character) const;
    std::optional<WarehouseData> loadWarehouseData(const std::string& account) const;
    bool saveInventoryData(const std::string& character, const InventoryData& data) const;
    bool saveWarehouseData(const std::string& account, const WarehouseData& data) const;
    void workerLoop();
    void enqueue(std::function<void()> task);

    PlayerRepository& m_players;
    CharacterService& m_characters;
    ItemPolicy& m_itemPolicy;
    InventorySqlRepository* m_sqlRepository = nullptr;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_worker;
    bool m_stopping = false;
};
