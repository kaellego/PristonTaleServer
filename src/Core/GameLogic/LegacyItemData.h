#pragma once

#include "Shared/datatypes.h"

#include <array>
#include <cstdint>
#include <optional>

#pragma pack(push, 1)
struct LegacyItemInfo {
    char name[64];
    char itemCode[32];
    char modelName[64];
    uint32_t itemCodeNumeric;
    int32_t itemType;
    int32_t itemClass;
    int32_t weight;
    int32_t price;
    int32_t level, strength, spirit, talent, dexterity, health;
    int32_t damageMin, damageMax, attackSpeed, attackRating, critical;
    int32_t defense, absorb, blockRating, moveSpeed;
    int32_t potionSpace, effectTime;
};

struct LegacyTransItemInfo {
    int32_t size;
    int32_t code;
    LegacyItemInfo item;
    int32_t x, y, z;
    uint32_t securityCode[4];
};
#pragma pack(pop)

static_assert(sizeof(LegacyItemInfo) == 248);
static_assert(sizeof(LegacyTransItemInfo) == 284);

// Layout isolado usado pelo adaptador legado. Não alterar Item sem migrar os
// arquivos .chr, os blobs do banco e os pacotes que já usam o modelo atual.
struct LegacyItemProperties {
    std::array<char, 32> name{};
    std::array<char, 16> code{};
    int32_t type = 0;
    int32_t weight = 0;
    int32_t price = 0;
    int32_t damageMin = 0;
    int32_t damageMax = 0;
    int32_t defense = 0;
    int32_t absorb = 0;
    int32_t levelRequirement = 1;
};

struct LegacyItemData {
    LegacyItemProperties properties{};
    int32_t uid = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 1;
    int32_t height = 1;
    int16_t durability = 0;
    int16_t maxDurability = 0;
    bool identified = false;
};

class ItemConverter final {
public:
    static bool validate(const LegacyItemData& item);
    static std::optional<Item> toCurrent(const LegacyItemData& item);
    static bool validate(const LegacyItemInfo& item);
    static std::optional<Item> toCurrent(const LegacyItemInfo& item);
    static LegacyItemData fromCurrent(const Item& item);
};
