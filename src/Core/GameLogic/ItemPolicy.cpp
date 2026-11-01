#include "GameLogic/ItemPolicy.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"

namespace {
struct ItemRule { int level; int slot; unsigned int jobs; int consumable; };

std::optional<ItemRule> loadRule(DatabasePool& database, int itemCode) {
    auto connection = database.createConnection(EDatabaseID::GameDB);
    connection->prepare("SELECT RequiredLevel, EquipmentSlot, JobCodeMask, Consumable FROM ItemList WHERE ItemCode = ?");
    connection->bindParameter(1, itemCode);
    connection->execute();
    if (!connection->fetch()) return std::nullopt;
    return ItemRule{connection->getData<int>(1).value_or(1), connection->getData<int>(2).value_or(-1),
                    static_cast<unsigned int>(connection->getData<long long>(3).value_or(0xFFFFFFFFLL)), connection->getData<int>(4).value_or(0)};
}
}

bool ItemPolicy::canEquip(int itemCode, int characterLevel, int characterClass, int equipmentSlot) const {
    auto rule = loadRule(m_database, itemCode);
    if (!rule || characterLevel < rule->level || characterClass < 1 || characterClass > 32) return false;
    if (rule->slot >= 0 && rule->slot != equipmentSlot) return false;
    return rule->jobs == 0xFFFFFFFFu || (rule->jobs & (1u << (characterClass - 1))) != 0;
}

bool ItemPolicy::canUse(int itemCode) const {
    auto rule = loadRule(m_database, itemCode);
    return rule && rule->consumable != 0;
}
