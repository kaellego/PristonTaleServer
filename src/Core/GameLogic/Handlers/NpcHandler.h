#pragma once

#include "Network/PacketDispatcher.h"

class GameWorld;
class LogService;
class InventoryService;
class NpcShopRepository;

class NpcHandler final : public IPacketHandler {
public:
    NpcHandler(GameWorld& world, InventoryService& inventory, NpcShopRepository& shops, LogService& logService)
        : m_world(world), m_inventory(inventory), m_shops(shops), m_logService(logService) {}
    void handle(std::shared_ptr<ClientSession> session, const Packet& packet) override;

private:
    GameWorld& m_world;
    InventoryService& m_inventory;
    NpcShopRepository& m_shops;
    LogService& m_logService;
};
