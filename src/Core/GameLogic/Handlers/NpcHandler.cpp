#include "GameLogic/Handlers/NpcHandler.h"

#include "GameLogic/GameWorld.h"
#include "GameLogic/NpcShopRepository.h"
#include "GameLogic/Services/InventoryService.h"
#include "Logging/LogService.h"
#include "Network/ClientSession.h"
#include "Network/LegacyGatewayProtocol.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"
#include "Utils/SafeString.h"

#include <cmath>
#include <cstring>
#include <vector>

void NpcHandler::handle(std::shared_ptr<ClientSession> session, const Packet &packet)
{
    m_logService.debug("Pedido NPC recebido: conta='{}' opcode=0x{:08x} bytes={} personagem='{}' autenticado={}",
                       session ? session->getAccountName() : "<null>", packet.header.opcode,
                       packet.body.size(), session ? session->getCharacterName() : "<null>",
                       session && session->isAuthenticated() ? 1 : 0);
    if (!session || !session->isAuthenticated() || session->ticketExpired() ||
        session->getCharacterName().empty())
    {
        if (session)
            session->close();
        return;
    }

    const bool shopRequest = packet.header.opcode == static_cast<uint32_t>(Opcodes::BuyItemNPCShop) ||
                             packet.header.opcode == static_cast<uint32_t>(Opcodes::SellItemNPCShop);
    if (shopRequest) {
        if (packet.body.size() != sizeof(PacketNpcShopRequest)) {
            m_logService.warn("Pedido de loja rejeitado para {}: tamanho={}",
                              session->getAccountName(), packet.body.size());
            return;
        }
        PacketNpcShopRequest request{};
        std::memcpy(&request, packet.body.data(), sizeof(request));
        if (request.itemCode <= 0 || request.quantity <= 0 || request.quantity > 2000000000 ||
            session->npcContextId() <= 0 || session->npcContextSerial() == 0) return;
        if (request.npcSerial != 0 && request.npcSerial != session->npcContextSerial()) return;
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

    if (packet.body.size() != sizeof(PacketNpcClick)) {
        m_logService.warn("Clique NPC rejeitado para {}: tamanho={}",
                          session->getAccountName(), packet.body.size());
        return;
    }

    PacketNpcClick click{};
    std::memcpy(&click, packet.body.data(), sizeof(click));
    if (click.objectSerial == 0 || click.npcId < 0)
        return;
    const auto entityId = m_world.entityIdForAccount(session->getAccountName());
    if (!entityId)
    {
        m_logService.warn("Pedido NPC rejeitado para {}: entidade do personagem nao encontrada.",
                          session->getAccountName());
        return;
    }
    m_logService.debug("Resolvendo NPC: conta='{}' entidade={} id={} serial=0x{:08x}",
                       session->getAccountName(), *entityId, click.npcId, click.objectSerial);
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
    m_logService.debug("NPC validado: conta='{}' id={} serial=0x{:08x} mapa={} pos=({}, {}, {}) modelo='{}'; enviando PLAYERINFO legado",
                       session->getAccountName(), npc->id, npc->objectSerial, npc->mapId,
                       data.x, data.y, data.z, npc->modelName);
    const auto *dataBytes = reinterpret_cast<const uint8_t *>(&data);
    // GETPLAYERINFO is the legacy handshake after PLAYDATAGROUP. Make the
    // response explicit instead of depending on a mutable session opcode.
    session->sendLegacyPacket(Packet(static_cast<uint32_t>(Opcodes::NpcData),
                                     std::vector<uint8_t>(dataBytes, dataBytes + sizeof(data))),
                              static_cast<int32_t>(LegacyGateway::Opcode::GetPlayerInfo));

    PacketNpcOpen opened{npc->id, npc->objectSerial, npc->eventCode, npc->mapId};
    session->setNpcContext(npc->id, npc->objectSerial);
    const auto *bytes = reinterpret_cast<const uint8_t *>(&opened);
    session->send(Packet(static_cast<uint32_t>(Opcodes::OpenNPC),
                         std::vector<uint8_t>(bytes, bytes + sizeof(opened))));

    const auto shopItems = m_shops.list(npc->id);
    for (int kind = 1; kind <= 3; ++kind) {
        std::vector<NpcShopItem> kindItems;
        for (const auto& item : shopItems)
            if (item.itemKind == kind)
                kindItems.push_back(item);
        if (kindItems.empty())
            continue;

        PacketNpcShopListHeader header{npc->id, kind, static_cast<int32_t>(kindItems.size())};
        std::vector<uint8_t> shopBody(sizeof(header) + kindItems.size() * sizeof(PacketNpcShopListEntry));
        std::memcpy(shopBody.data(), &header, sizeof(header));
        for (size_t i = 0; i < kindItems.size(); ++i) {
            PacketNpcShopListEntry entry{};
            entry.itemCode = kindItems[i].itemCode;
            entry.buyPrice = kindItems[i].buyPrice;
            entry.sellPrice = kindItems[i].sellPrice;
            entry.requiredLevel = kindItems[i].requiredLevel;
            copyFixed(entry.itemName, kindItems[i].itemName);
            std::memcpy(shopBody.data() + sizeof(header) + i * sizeof(entry), &entry, sizeof(entry));
        }
        session->sendLegacyPacket(Packet(static_cast<uint32_t>(Opcodes::Shop_ItemList), std::move(shopBody)),
                                  static_cast<int32_t>(LegacyGateway::Opcode::ShopItemList));
        m_logService.debug("Catalogo legado enviado: NPC={} tipo={} itens={}",
                           npc->id, kind, kindItems.size());
    }
    if (shopItems.empty())
        m_logService.warn("NPC '{}' nao possui itens de loja habilitados", npc->name);
    m_logService.info("NPC '{}' aberto para {} (id={})", npc->name, session->getAccountName(), npc->id);
}
