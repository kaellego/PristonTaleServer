#pragma once
#include "Network/PacketDispatcher.h"
class InventoryService;
class GameWorld;
class LogService;
class ItemHandler final : public IPacketHandler
{
public:
    ItemHandler(InventoryService& inventory, GameWorld& world, LogService& log) : m_inventory(inventory), m_world(world), m_log(log) {}
    void handle(std::shared_ptr<ClientSession> session, const Packet &packet) override;

private:
    InventoryService& m_inventory;
    GameWorld& m_world;
    LogService &m_log;
};
