#pragma once

#include "Shared/datatypes.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class CharacterService;
class PlayerRepository;
class ItemPolicy;

class InventoryService {
public:
    using InventoryCallback = std::function<void(bool, InventoryData)>;
    using WarehouseCallback = std::function<void(bool, WarehouseData)>;
    using TransferCallback = std::function<void(bool, InventoryData, WarehouseData)>;
    using ItemCallback = std::function<void(bool, InventoryData, Item)>;

    InventoryService(PlayerRepository& players, CharacterService& characters, ItemPolicy& itemPolicy);
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
    void workerLoop();
    void enqueue(std::function<void()> task);

    PlayerRepository& m_players;
    CharacterService& m_characters;
    ItemPolicy& m_itemPolicy;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_worker;
    bool m_stopping = false;
};
