#pragma once
#include "Network/PacketDispatcher.h"
class InventoryService;
class LogService;
class ItemHandler final : public IPacketHandler
{
public:
    ItemHandler(InventoryService& inventory, LogService& log) : m_inventory(inventory), m_log(log) {}
    void handle(std::shared_ptr<ClientSession> session, const Packet &packet) override;

private:
    InventoryService& m_inventory;
    LogService &m_log;
};
