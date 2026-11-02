#include "GameLogic/Handlers/NpcHandler.h"

#include "GameLogic/GameWorld.h"
#include "GameLogic/NpcShopRepository.h"
#include "GameLogic/Services/InventoryService.h"
#include "Logging/LogService.h"
#include "Network/ClientSession.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Utils/SafeString.h"

#include <cmath>
#include <cstring>

void NpcHandler::handle(std::shared_ptr<ClientSession> session, const Packet &packet)
{
    if (!session || !session->isAuthenticated() || session->ticketExpired() ||
        session->getCharacterName().empty() || packet.body.size() != sizeof(PacketNpcClick))
    {
        if (session)
            session->close();
        return;
    }

    if (packet.header.opcode == static_cast<uint32_t>(Opcodes::BuyItemNPCShop) ||
        packet.header.opcode == static_cast<uint32_t>(Opcodes::SellItemNPCShop)) {
        if (packet.body.size() != sizeof(PacketNpcShopRequest)) return;
        PacketNpcShopRequest request{};
        std::memcpy(&request, packet.body.data(), sizeof(request));
        if (request.itemCode <= 0 || request.quantity <= 0 || request.quantity > 2000000000 ||
            session->npcContextId() <= 0 || session->npcContextSerial() == 0) return;
        const auto shopItem = m_shops.find(session->npcContextId(), request.itemCode);
        if (!shopItem || (shopItem->stock >= 0 && request.quantity > shopItem->stock)) return;
        const bool buying = packet.header.opcode == static_cast<uint32_t>(Opcodes::BuyItemNPCShop);
        const auto price = buying ? shopItem->buyPrice : shopItem->sellPrice;
        if (price < 0) return;
        const auto callback = [session](bool ok, InventoryData data) {
            if (!ok) return;
            const auto* bytes = reinterpret_cast<const uint8_t*>(&data);
            session->send(Packet(static_cast<uint32_t>(Opcodes::NetPlayerInventory),
                                 std::vector<uint8_t>(bytes, bytes + sizeof(data))));
        };
        if (buying) m_inventory.buyNpcItem(session->getAccountName(), session->getCharacterName(), request.itemCode, request.quantity, price, callback);
        else m_inventory.sellNpcItem(session->getAccountName(), session->getCharacterName(), request.itemCode, request.quantity, price, callback);
        return;
    }

    PacketNpcClick click{};
    std::memcpy(&click, packet.body.data(), sizeof(click));
    if (click.objectSerial == 0 || click.npcId < 0)
        return;
    const auto entityId = m_world.entityIdForAccount(session->getAccountName());
    if (!entityId)
        return;
    const auto npc = m_world.interactNpc(*entityId, click.npcId, click.objectSerial);
    if (!npc)
    {
        m_logService.warn("Interacao NPC rejeitada para {}: id={}, serial=0x{:08x}",
                          session->getAccountName(), click.npcId, click.objectSerial);
        return;
    }

    PacketNpcData data{};
    data.npcId = npc->id;
    data.objectSerial = npc->objectSerial;
    data.mapId = npc->mapId;
    data.x = static_cast<int32_t>(std::lround(npc->position.x));
    data.y = static_cast<int32_t>(std::lround(npc->position.y));
    data.z = static_cast<int32_t>(std::lround(npc->position.z));
    data.angleY = npc->angleY;
    data.eventCode = npc->eventCode;
    data.enabled = npc->enabled ? 1 : 0;
    copyFixed(data.szName, npc->name);
    copyFixed(data.szModelName, npc->modelName);
    const auto *dataBytes = reinterpret_cast<const uint8_t *>(&data);
    session->send(Packet(static_cast<uint32_t>(Opcodes::NpcData),
                         std::vector<uint8_t>(dataBytes, dataBytes + sizeof(data))));

    PacketNpcOpen opened{npc->id, npc->objectSerial, npc->eventCode, npc->mapId};
    session->setNpcContext(npc->id, npc->objectSerial);
    const auto *bytes = reinterpret_cast<const uint8_t *>(&opened);
    session->send(Packet(static_cast<uint32_t>(Opcodes::OpenNPC),
                         std::vector<uint8_t>(bytes, bytes + sizeof(opened))));
    m_logService.info("NPC '{}' aberto para {} (id={})", npc->name, session->getAccountName(), npc->id);
}
