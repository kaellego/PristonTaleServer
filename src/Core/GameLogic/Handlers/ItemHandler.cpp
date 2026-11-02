#include "GameLogic/Handlers/ItemHandler.h"
#include "GameLogic/Services/InventoryService.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "GameLogic/GameWorld.h"
#include <cstring>
#include <utility>

namespace
{
    template <typename T>
    void sendData(const std::shared_ptr<ClientSession> &s, uint32_t op, const T &data)
    {
        const auto *p = reinterpret_cast<const uint8_t *>(&data) + sizeof(PacketHeader);
        s->send(Packet(op, std::vector<uint8_t>(p, p + sizeof(T) - sizeof(PacketHeader))));
    }
}

void ItemHandler::handle(std::shared_ptr<ClientSession> session, const Packet &packet)
{
    if (!session->isAuthenticated() || session->ticketExpired() || session->getCharacterName().empty())
    {
        session->close();
        return;
    }
    const auto op = static_cast<Opcodes>(packet.header.opcode);
    if (op == Opcodes::NetPlayerInventory || op == Opcodes::NetOpenWarehouse || op == Opcodes::NetOpenWarehouseSeasonal ||
        op == Opcodes::RecvWarehouse || op == Opcodes::SendWarehouse || op == Opcodes::ItemSlotHandle ||
        op == Opcodes::UseItemCode || op == Opcodes::CheckItemUseSlot || op == Opcodes::CheckItemUseSlotEx ||
        op == Opcodes::NetWarehouseTransfer || op == Opcodes::NetDropItem || op == Opcodes::NetPickupDrop) {
        if (!session->acceptItemOperation(packet.header.opcode, packet.body)) return;
    }
    if (op == Opcodes::NetPlayerInventory)
    {
        m_inventory.loadInventory(session->getAccountName(), session->getCharacterName(), [session, opcode = packet.header.opcode](bool ok, InventoryData data) {
            if (ok) sendData(session, opcode, data); else session->close();
        });
        return;
    }
    if (op == Opcodes::NetOpenWarehouse || op == Opcodes::NetOpenWarehouseSeasonal || op == Opcodes::RecvWarehouse || op == Opcodes::SendWarehouse)
    {
        m_inventory.loadWarehouse(session->getAccountName(), session->getCharacterName(), [session](bool ok, WarehouseData data) {
            if (ok) sendData(session, static_cast<uint32_t>(Opcodes::RecvWarehouse), data); else session->close();
        });
        return;
    }
    if (op == Opcodes::ItemSlotHandle && packet.body.size() == sizeof(PacketItemSlotHandle) - sizeof(PacketHeader))
    {
        PacketItemSlotHandle command{};
        std::memcpy(reinterpret_cast<uint8_t *>(&command) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
        if (command.sourceSlot < 0 || command.sourceSlot >= static_cast<int32_t>(PLAYER_INVENTORY_SLOTS) || command.targetSlot < 0 || command.targetSlot >= static_cast<int32_t>(PLAYER_INVENTORY_SLOTS))
            return;
        m_inventory.moveInventoryItem(session->getAccountName(), session->getCharacterName(), command.sourceSlot, command.targetSlot, [session](bool ok, InventoryData data) {
            if (ok) sendData(session, static_cast<uint32_t>(Opcodes::NetPlayerInventory), data); else session->close();
        });
    }
    if ((op == Opcodes::UseItemCode || op == Opcodes::CheckItemUseSlot || op == Opcodes::CheckItemUseSlotEx) && packet.body.size() == sizeof(PacketItemUse) - sizeof(PacketHeader))
    {
        PacketItemUse command{};
        std::memcpy(reinterpret_cast<uint8_t*>(&command) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
        m_inventory.useOrEquipItem(session->getAccountName(), session->getCharacterName(), command.inventorySlot, command.equipmentSlot, command.action, [session](bool ok, InventoryData data) {
            if (ok) sendData(session, static_cast<uint32_t>(Opcodes::NetPlayerInventory), data); else session->close();
        });
    }
    if (op == Opcodes::NetWarehouseTransfer && packet.body.size() == sizeof(PacketWarehouseTransfer) - sizeof(PacketHeader))
    {
        PacketWarehouseTransfer command{};
        std::memcpy(reinterpret_cast<uint8_t*>(&command) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
        if (command.toWarehouse != 0 && command.toWarehouse != 1) return;
        m_inventory.transferItem(session->getAccountName(), session->getCharacterName(), command.inventorySlot, command.warehouseSlot, command.quantity, command.toWarehouse != 0,
            [session](bool ok, InventoryData inventory, WarehouseData warehouse) {
                if (!ok) { session->close(); return; }
                sendData(session, static_cast<uint32_t>(Opcodes::NetPlayerInventory), inventory);
                sendData(session, static_cast<uint32_t>(Opcodes::RecvWarehouse), warehouse);
            });
    }
    if (op == Opcodes::NetDropItem && packet.body.size() == sizeof(PacketDropRequest) - sizeof(PacketHeader)) {
        PacketDropRequest command{}; std::memcpy(reinterpret_cast<uint8_t*>(&command) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
        m_inventory.dropItem(session->getAccountName(), session->getCharacterName(), command.inventorySlot, command.quantity, [this, session](bool ok, InventoryData data, Item item) {
            if (!ok) { session->close(); return; }
            auto entity = m_world.entityIdForAccount(session->getAccountName());
            if (!entity || !m_world.spawnDrop(*entity, item)) { session->close(); return; }
            sendData(session, static_cast<uint32_t>(Opcodes::NetPlayerInventory), data);
        });
    }
    if (op == Opcodes::NetPickupDrop && packet.body.size() == sizeof(PacketDropPickup) - sizeof(PacketHeader)) {
        PacketDropPickup command{}; std::memcpy(reinterpret_cast<uint8_t*>(&command) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
        auto entity = m_world.entityIdForAccount(session->getAccountName()); if (!entity) { session->close(); return; }
        auto item = m_world.takeDrop(*entity, command.dropId); if (!item) return;
        m_inventory.addItem(session->getAccountName(), session->getCharacterName(), *item, [this, session, entity, item = *item](bool ok, InventoryData data) { if (ok) sendData(session, static_cast<uint32_t>(Opcodes::NetPlayerInventory), data); else { m_world.restoreDrop(item, *entity); session->close(); } });
    }
}
