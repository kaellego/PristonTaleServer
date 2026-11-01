#include "GameLogic/LegacyItemData.h"

#include <algorithm>
#include <charconv>
#include <cstring>

namespace {
bool hasTerminator(const auto& value) {
    return std::find(value.begin(), value.end(), '\0') != value.end();
}
}

bool ItemConverter::validate(const LegacyItemData& item) {
    if (!hasTerminator(item.properties.name) || !hasTerminator(item.properties.code))
        return false;
    if (item.uid <= 0 || item.width <= 0 || item.height <= 0 || item.width > 16 || item.height > 16)
        return false;
    if (item.x < 0 || item.y < 0 || item.durability < 0 || item.maxDurability < 0 ||
        item.durability > item.maxDurability)
        return false;
    if (item.properties.type < 0 || item.properties.weight < 0 || item.properties.price < 0 ||
        item.properties.levelRequirement < 1 || item.properties.levelRequirement > 200 ||
        item.properties.damageMin < 0 || item.properties.damageMax < item.properties.damageMin ||
        item.properties.defense < 0 || item.properties.absorb < 0)
        return false;
    return true;
}

bool ItemConverter::validate(const LegacyItemInfo& item) {
    if (item.itemCodeNumeric == 0 || std::memchr(item.name, '\0', sizeof(item.name)) == nullptr ||
        std::memchr(item.itemCode, '\0', sizeof(item.itemCode)) == nullptr ||
        std::memchr(item.modelName, '\0', sizeof(item.modelName)) == nullptr)
        return false;
    if (item.itemType < 0 || item.itemType > 32 || item.itemClass < 0 || item.itemClass > 11 ||
        item.weight < 0 || item.price < 0 || item.level < 1 || item.level > 200 ||
        item.damageMin < 0 || item.damageMax < item.damageMin || item.defense < 0 || item.absorb < 0 ||
        item.potionSpace < 0 || item.effectTime < 0)
        return false;
    return true;
}

std::optional<Item> ItemConverter::toCurrent(const LegacyItemData& item) {
    if (!validate(item))
        return std::nullopt;

    int32_t itemId = 0;
    const auto* first = item.properties.code.data();
    const auto* last = first + std::strlen(first);
    const auto parsed = std::from_chars(first, last, itemId);
    // O modelo atual só possui ID numérico; códigos legados como WA133
    // precisam de uma tabela ItemList antes de poderem ser persistidos.
    if (parsed.ec != std::errc{} || parsed.ptr != last || itemId <= 0)
        return std::nullopt;

    Item result{};
    result.sItemID = itemId;
    std::memcpy(result.szItemName, item.properties.name.data(), sizeof(result.szItemName));
    return result;
}

std::optional<Item> ItemConverter::toCurrent(const LegacyItemInfo& item) {
    if (!validate(item))
        return std::nullopt;
    Item result{};
    result.sItemID = static_cast<int32_t>(item.itemCodeNumeric);
    std::memcpy(result.szItemName, item.name, sizeof(result.szItemName));
    return result;
}

LegacyItemData ItemConverter::fromCurrent(const Item& item) {
    LegacyItemData result{};
    result.uid = item.sItemID;
    std::memcpy(result.properties.name.data(), item.szItemName, result.properties.name.size());
    return result;
}
