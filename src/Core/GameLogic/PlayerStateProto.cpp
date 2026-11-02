#include "GameLogic/PlayerStateProto.h"

#include "GameLogic/Generated/player_state.pb.h"

#include <cstring>
#include <string>

namespace PlayerStateProto {
namespace {
constexpr std::size_t MaxBlob = 1024 * 1024;

std::size_t textSize(const char* value, std::size_t capacity) {
    std::size_t size = 0;
    while (size + 1 < capacity && value[size] != '\0') ++size;
    return size;
}

template <typename Message>
std::vector<uint8_t> serialize(const Message& message) {
    std::string serialized;
    if (!message.SerializeToString(&serialized) || serialized.empty() || serialized.size() > MaxBlob)
        return {};
    return std::vector<uint8_t>(serialized.begin(), serialized.end());
}

void encodeItem(const Item& source, zeropt::Item& target) {
    target.set_item_id(source.sItemID);
    target.set_item_name(std::string(source.szItemName, textSize(source.szItemName, sizeof(source.szItemName))));
    target.set_quantity(source.iQuantity);
    target.set_serial(source.serial);
    target.set_checksum(source.checksum);
    target.set_version(source.version);
    target.set_hp_regen(source.fHPRegen);
    target.set_mp_regen(source.fMPRegen);
    target.set_sp_regen(source.fSPRegen);
}

bool decodeItem(const zeropt::Item& source, Item& target) {
    if (source.item_name().size() >= sizeof(target.szItemName)) return false;
    target = {};
    target.sItemID = source.item_id();
    std::memcpy(target.szItemName, source.item_name().data(), source.item_name().size());
    target.iQuantity = source.quantity();
    target.serial = source.serial();
    target.checksum = source.checksum();
    target.version = source.version();
    target.fHPRegen = source.hp_regen();
    target.fMPRegen = source.mp_regen();
    target.fSPRegen = source.sp_regen();
    return target.sItemID >= 0 && target.iQuantity >= 0;
}

template <typename Message, typename Items>
void encodeItems(const Items& source, int32_t gold, Message& target) {
    target.set_gold(gold);
    for (const auto& item : source) encodeItem(item, *target.add_item());
}

template <typename Message, typename Items>
bool decodeItems(const Message& source, Items& target, int32_t& gold) {
    if (source.item_size() > static_cast<int>(std::size(target))) return false;
    for (auto& item : target) item = {};
    gold = source.gold();
    for (int i = 0; i < source.item_size(); ++i)
        if (!decodeItem(source.item(i), target[static_cast<std::size_t>(i)])) return false;
    return true;
}
}

std::vector<uint8_t> encode(const CharacterData& value) {
    zeropt::CharacterState message;
    message.set_name(std::string(value.szName, textSize(value.szName, sizeof(value.szName))));
    message.set_level(value.iLevel); message.set_experience(value.liEXP); message.set_class_id(value.iClass); message.set_map_id(value.iMapId);
    message.set_pos_x(value.fPosX); message.set_pos_y(value.fPosY); message.set_pos_z(value.fPosZ);
    message.set_hp(value.iHP); message.set_max_hp(value.iMaxHP); message.set_mp(value.iMP); message.set_max_mp(value.iMaxMP);
    message.set_stm(value.iSTM); message.set_max_stm(value.iMaxSTM);
    message.set_hp_regen(value.fHPRegen); message.set_mp_regen(value.fMPRegen); message.set_sp_regen(value.fSPRegen);
    message.set_hp_regen_bonus(value.fHPRegenBonus); message.set_mp_regen_bonus(value.fMPRegenBonus); message.set_sp_regen_bonus(value.fSPRegenBonus);
    message.set_strength(value.iStrength); message.set_spirit(value.iSpirit); message.set_talent(value.iTalent); message.set_agility(value.iAgility); message.set_health(value.iHealth);
    message.set_attack_min(value.iAttackMin); message.set_attack_max(value.iAttackMax); message.set_defense(value.iDefense); message.set_gold(value.iGold);
    for (const auto& item : value.saEquipment) encodeItem(item, *message.add_equipment());
    return serialize(message);
}

std::vector<uint8_t> encode(const InventoryData& value) {
    zeropt::InventoryState message;
    encodeItems(value.saItem, value.iGold, message);
    return serialize(message);
}

std::vector<uint8_t> encode(const WarehouseData& value) {
    zeropt::WarehouseState message;
    encodeItems(value.saItem, value.iGold, message);
    return serialize(message);
}

std::optional<CharacterData> decodeCharacter(const std::vector<uint8_t>& bytes) {
    if (bytes.empty() || bytes.size() > MaxBlob) return std::nullopt;
    zeropt::CharacterState message;
    CharacterData value{};
    if (!message.ParseFromArray(bytes.data(), static_cast<int>(bytes.size())) || message.name().size() >= sizeof(value.szName)) return std::nullopt;
    std::memcpy(value.szName, message.name().data(), message.name().size());
    value.iLevel = message.level(); value.liEXP = message.experience(); value.iClass = message.class_id(); value.iMapId = message.map_id();
    value.fPosX = message.pos_x(); value.fPosY = message.pos_y(); value.fPosZ = message.pos_z();
    value.iHP = message.hp(); value.iMaxHP = message.max_hp(); value.iMP = message.mp(); value.iMaxMP = message.max_mp(); value.iSTM = message.stm(); value.iMaxSTM = message.max_stm();
    value.fHPRegen = message.hp_regen(); value.fMPRegen = message.mp_regen(); value.fSPRegen = message.sp_regen();
    value.fHPRegenBonus = message.hp_regen_bonus(); value.fMPRegenBonus = message.mp_regen_bonus(); value.fSPRegenBonus = message.sp_regen_bonus();
    value.iStrength = message.strength(); value.iSpirit = message.spirit(); value.iTalent = message.talent(); value.iAgility = message.agility(); value.iHealth = message.health();
    value.iAttackMin = message.attack_min(); value.iAttackMax = message.attack_max(); value.iDefense = message.defense(); value.iGold = message.gold();
    if (message.equipment_size() > static_cast<int>(std::size(value.saEquipment))) return std::nullopt;
    for (int i = 0; i < message.equipment_size(); ++i)
        if (!decodeItem(message.equipment(i), value.saEquipment[static_cast<std::size_t>(i)])) return std::nullopt;
    return value;
}

std::optional<InventoryData> decodeInventory(const std::vector<uint8_t>& bytes) {
    if (bytes.empty() || bytes.size() > MaxBlob) return std::nullopt;
    zeropt::InventoryState message;
    if (!message.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) return std::nullopt;
    InventoryData value{};
    if (!decodeItems(message, value.saItem, value.iGold)) return std::nullopt;
    return value;
}

std::optional<WarehouseData> decodeWarehouse(const std::vector<uint8_t>& bytes) {
    if (bytes.empty() || bytes.size() > MaxBlob) return std::nullopt;
    zeropt::WarehouseState message;
    if (!message.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) return std::nullopt;
    WarehouseData value{};
    if (!decodeItems(message, value.saItem, value.iGold)) return std::nullopt;
    return value;
}
}
