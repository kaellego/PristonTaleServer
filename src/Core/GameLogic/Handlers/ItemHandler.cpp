#include "GameLogic/Handlers/ItemHandler.h"
#include "GameLogic/PlayerRepository.h"
#include "GameLogic/Services/CharacterService.h"
#include "Network/ClientSession.h"
#include "Network/Packet.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include <cstring>
#include <utility>

namespace { template <typename T> void sendData(const std::shared_ptr<ClientSession>& s, uint32_t op, const T& data) {
    const auto* p = reinterpret_cast<const uint8_t*>(&data) + sizeof(PacketHeader);
    s->send(Packet(op, std::vector<uint8_t>(p, p + sizeof(T) - sizeof(PacketHeader))));
} }

void ItemHandler::handle(std::shared_ptr<ClientSession> session, const Packet& packet) {
    if (!session->isAuthenticated() || session->getCharacterName().empty() || !m_characters.ownsCharacter(session->getAccountName(), session->getCharacterName())) { session->close(); return; }
    const auto op = static_cast<Opcodes>(packet.header.opcode);
    if (op == Opcodes::NetPlayerInventory) {
        auto data = m_players.loadInventory(session->getCharacterName()).value_or(InventoryData{});
        m_players.saveInventory(session->getCharacterName(), data); sendData(session, packet.header.opcode, data); return;
    }
    if (op == Opcodes::NetOpenWarehouse || op == Opcodes::NetOpenWarehouseSeasonal || op == Opcodes::RecvWarehouse || op == Opcodes::SendWarehouse) {
        auto data = m_players.loadWarehouse(session->getAccountName()).value_or(WarehouseData{});
        m_players.saveWarehouse(session->getAccountName(), data); sendData(session, static_cast<uint32_t>(Opcodes::RecvWarehouse), data); return;
    }
    if (op == Opcodes::ItemSlotHandle && packet.body.size() == sizeof(PacketItemSlotHandle) - sizeof(PacketHeader)) {
        PacketItemSlotHandle command{}; std::memcpy(reinterpret_cast<uint8_t*>(&command) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
        if (command.sourceSlot < 0 || command.sourceSlot >= static_cast<int32_t>(PLAYER_INVENTORY_SLOTS) || command.targetSlot < 0 || command.targetSlot >= static_cast<int32_t>(PLAYER_INVENTORY_SLOTS)) return;
        auto data = m_players.loadInventory(session->getCharacterName()).value_or(InventoryData{}); std::swap(data.saItem[command.sourceSlot], data.saItem[command.targetSlot]);
        if (m_players.saveInventory(session->getCharacterName(), data)) sendData(session, static_cast<uint32_t>(Opcodes::NetPlayerInventory), data);
    }
}
