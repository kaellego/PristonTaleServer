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
            db->getData<int>(5).value_or(-1)};
    } catch (...) { return std::nullopt; }
}
