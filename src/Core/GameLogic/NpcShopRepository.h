#pragma once

#include <optional>
#include <string>
#include <vector>

class DatabasePool;

struct NpcShopItem {
    int itemCode = 0;
    long long buyPrice = 0;
    long long sellPrice = 0;
    int requiredLevel = 1;
    int stock = -1;
    std::string itemName;
    int itemKind = 3;
};

class NpcShopRepository final {
public:
    explicit NpcShopRepository(DatabasePool& pool) : m_pool(pool) {}
    std::optional<NpcShopItem> find(int npcId, int itemCode) const;
    std::vector<NpcShopItem> list(int npcId) const;

private:
    DatabasePool& m_pool;
};
