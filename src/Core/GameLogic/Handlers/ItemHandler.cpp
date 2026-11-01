#include "GameLogic/Handlers/ItemHandler.h"
#include "GameLogic/Services/InventoryService.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
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
    if (!session->isAuthenticated() || session->getCharacterName().empty())
    {
        session->close();
        return;
    }
    const auto op = static_cast<Opcodes>(packet.header.opcode);
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
}
