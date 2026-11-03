#include "GameLogic/NpcShopRepository.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"

std::optional<NpcShopItem> NpcShopRepository::find(int npcId, int itemCode) const {
    try {
        auto db = m_pool.createConnection(EDatabaseID::GameDB);
        db->prepare(R"SQL(
            SELECT i.ItemCode, i.BuyPrice, i.SellPrice, i.RequiredLevel, i.Stock
              FROM dbo.NpcShop AS s
              INNER JOIN dbo.NpcShopItem AS i ON i.ShopId = s.ShopId
             WHERE s.NpcId=? AND s.Enabled=1 AND i.ItemCode=? AND i.Enabled=1
        )SQL");
        db->bindParameter(1, npcId);
        db->bindParameter(2, itemCode);
        db->execute();
        if (!db->fetch()) return std::nullopt;
        return NpcShopItem{
            db->getData<int>(1).value_or(0),
            db->getData<long long>(2).value_or(-1),
            db->getData<long long>(3).value_or(0),
            db->getData<int>(4).value_or(1),
            db->getData<int>(5).value_or(-1),
            {},
            3};
    } catch (...) { return std::nullopt; }
}

std::vector<NpcShopItem> NpcShopRepository::list(int npcId) const {
    std::vector<NpcShopItem> result;
    try {
        auto db = m_pool.createConnection(EDatabaseID::GameDB);
        db->prepare(R"SQL(
            SELECT i.ItemCode, i.BuyPrice, i.SellPrice, i.RequiredLevel, i.Stock,
                   COALESCE(l.ItemName, ''), COALESCE(l.EquipmentSlot, -2),
                   COALESCE(l.Consumable, 0)
              FROM dbo.NpcShop AS s
              INNER JOIN dbo.NpcShopItem AS i ON i.ShopId = s.ShopId
              LEFT JOIN dbo.ItemList AS l ON l.ItemCode = i.ItemCode
             WHERE s.NpcId=? AND s.Enabled=1 AND i.Enabled=1
             ORDER BY i.ItemCode
        )SQL");
        db->bindParameter(1, npcId);
        db->execute();
        while (db->fetch()) {
            NpcShopItem item{};
            item.itemCode = db->getData<int>(1).value_or(0);
            item.buyPrice = db->getData<long long>(2).value_or(-1);
            item.sellPrice = db->getData<long long>(3).value_or(0);
            item.requiredLevel = db->getData<int>(4).value_or(1);
            item.stock = db->getData<int>(5).value_or(-1);
            item.itemName = db->getData<std::string>(6).value_or("");
            const int slot = db->getData<int>(7).value_or(-2);
            const int consumable = db->getData<int>(8).value_or(0);
            item.itemKind = consumable ? 3 : (slot == 0 || slot == 1 ? 1 : 2);
            if (item.itemCode > 0 && item.buyPrice >= 0)
                result.push_back(std::move(item));
        }
    } catch (...) {
        result.clear();
    }
    return result;
}
