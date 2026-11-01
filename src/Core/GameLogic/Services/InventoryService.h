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

class InventoryService {
public:
    using InventoryCallback = std::function<void(bool, InventoryData)>;
    using WarehouseCallback = std::function<void(bool, WarehouseData)>;

    InventoryService(PlayerRepository& players, CharacterService& characters);
    ~InventoryService();
    InventoryService(const InventoryService&) = delete;

    void loadInventory(const std::string& account, const std::string& character, InventoryCallback callback);
    void loadWarehouse(const std::string& account, const std::string& character, WarehouseCallback callback);
    void moveInventoryItem(const std::string& account, const std::string& character, int source, int target, InventoryCallback callback);
    void useOrEquipItem(const std::string& account, const std::string& character, int inventorySlot, int equipmentSlot, int action, InventoryCallback callback);

private:
    void workerLoop();
    void enqueue(std::function<void()> task);

    PlayerRepository& m_players;
    CharacterService& m_characters;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_worker;
    bool m_stopping = false;
};
