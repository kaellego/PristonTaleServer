#include "GameLogic/PlayerStateProto.h"

#include "GameLogic/Generated/player_state.pb.h"

#include <cstring>
#include <algorithm>
#include <cmath>
#include <string>

namespace PlayerStateProto {
namespace {
constexpr std::size_t MaxBlob = 1024 * 1024;
constexpr std::array<uint8_t, 4> StateMagic{'Z', 'P', 'T', '2'};
constexpr uint32_t StateVersion = 2;

std::vector<uint8_t> v2Envelope(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> result;
    result.reserve(12 + payload.size());
    result.insert(result.end(), StateMagic.begin(), StateMagic.end());
    for (int i = 0; i < 4; ++i) result.push_back(static_cast<uint8_t>(StateVersion >> (i * 8)));
    const uint32_t size = static_cast<uint32_t>(payload.size());
    for (int i = 0; i < 4; ++i) result.push_back(static_cast<uint8_t>(size >> (i * 8)));
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

std::optional<std::vector<uint8_t>> unwrapState(const std::vector<uint8_t>& bytes) {
    if (bytes.size() >= 12 && std::equal(StateMagic.begin(), StateMagic.end(), bytes.begin())) {
        uint32_t version = 0, size = 0;
        std::memcpy(&version, bytes.data() + 4, sizeof(version));
        std::memcpy(&size, bytes.data() + 8, sizeof(size));
        if (version != StateVersion || size == 0 || size > MaxBlob - 12 || bytes.size() != 12u + size)
            return std::nullopt;
        return std::vector<uint8_t>(bytes.begin() + 12, bytes.end());
    }
    // V1 was a raw protobuf message. It remains readable and is upgraded on save.
    return bytes;
}

void normalizeVitals(CharacterData& value) {
    if (value.iClass < 1 || value.iClass > 11) value.iClass = 1;
    value.iLevel = std::clamp(value.iLevel, 1, 150);
    if (!std::isfinite(value.fPosX)) value.fPosX = 0.0f;
    if (!std::isfinite(value.fPosY)) value.fPosY = 0.0f;
    if (!std::isfinite(value.fPosZ)) value.fPosZ = 0.0f;
    if (value.iMapId < 0 || value.iMapId > 255) value.iMapId = 0;
    if (value.iMaxHP < 100) value.iMaxHP = 100;
    if (value.iMaxMP < 100) value.iMaxMP = 100;
    if (value.iMaxSTM < 100) value.iMaxSTM = 100;
    if (value.iHP <= 0) value.iHP = value.iMaxHP;
    if (value.iMP <= 0) value.iMP = value.iMaxMP;
    if (value.iSTM <= 0) value.iSTM = value.iMaxSTM;
    value.iHP = std::clamp(value.iHP, 0, value.iMaxHP);
    value.iMP = std::clamp(value.iMP, 0, value.iMaxMP);
    value.iSTM = std::clamp(value.iSTM, 0, value.iMaxSTM);
}

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
    return v2Envelope(std::vector<uint8_t>(serialized.begin(), serialized.end()));
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
    target.set_item_type(source.itemType);
    target.set_item_class(source.itemClass);
    target.set_required_level(source.requiredLevel);
    target.set_damage_min(source.damageMin);
    target.set_damage_max(source.damageMax);
    target.set_defense(source.defense);
    target.set_absorb(source.absorb);
    target.set_durability(source.durability);
    target.set_max_durability(source.maxDurability);
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
    target.itemType = source.item_type();
    target.itemClass = source.item_class();
    target.requiredLevel = source.required_level();
    target.damageMin = source.damage_min();
    target.damageMax = source.damage_max();
    target.defense = source.defense();
    target.absorb = source.absorb();
    target.durability = source.durability();
    target.maxDurability = source.max_durability();
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
    message.set_accuracy(value.iAccuracy); message.set_attack_rating(value.iAttackRating);
    message.set_critical(value.iCritical); message.set_attack_speed(value.iAttackSpeed);
    message.set_absorption(value.iAbsorption); message.set_block_rating(value.iBlockRating);
    message.set_move_speed(value.iMoveSpeed); message.set_sight(value.iSight);
    for (const auto resistance : value.iResistance) message.add_resistance(resistance);
    for (const auto resistance : value.iAttackResistance) message.add_attack_resistance(resistance);
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
    const auto payload = unwrapState(bytes);
    if (!payload) return std::nullopt;
    zeropt::CharacterState message;
    CharacterData value{};
    if (!message.ParseFromArray(payload->data(), static_cast<int>(payload->size())) || message.name().size() >= sizeof(value.szName)) return std::nullopt;
    std::memcpy(value.szName, message.name().data(), message.name().size());
    value.iLevel = message.level(); value.liEXP = message.experience(); value.iClass = message.class_id(); value.iMapId = message.map_id();
    value.fPosX = message.pos_x(); value.fPosY = message.pos_y(); value.fPosZ = message.pos_z();
    value.iHP = message.hp(); value.iMaxHP = message.max_hp(); value.iMP = message.mp(); value.iMaxMP = message.max_mp(); value.iSTM = message.stm(); value.iMaxSTM = message.max_stm();
    value.fHPRegen = message.hp_regen(); value.fMPRegen = message.mp_regen(); value.fSPRegen = message.sp_regen();
    value.fHPRegenBonus = message.hp_regen_bonus(); value.fMPRegenBonus = message.mp_regen_bonus(); value.fSPRegenBonus = message.sp_regen_bonus();
    value.iStrength = message.strength(); value.iSpirit = message.spirit(); value.iTalent = message.talent(); value.iAgility = message.agility(); value.iHealth = message.health();
    value.iAttackMin = message.attack_min(); value.iAttackMax = message.attack_max(); value.iDefense = message.defense(); value.iGold = message.gold();
    value.iAccuracy = message.accuracy(); value.iAttackRating = message.attack_rating();
    value.iCritical = message.critical(); value.iAttackSpeed = message.attack_speed();
    value.iAbsorption = message.absorption(); value.iBlockRating = message.block_rating();
    value.iMoveSpeed = message.move_speed(); value.iSight = message.sight();
    if (message.resistance_size() > static_cast<int>(std::size(value.iResistance)) ||
        message.attack_resistance_size() > static_cast<int>(std::size(value.iAttackResistance))) return std::nullopt;
    for (int i = 0; i < message.resistance_size(); ++i) value.iResistance[i] = message.resistance(i);
    for (int i = 0; i < message.attack_resistance_size(); ++i) value.iAttackResistance[i] = message.attack_resistance(i);
    if (message.equipment_size() > static_cast<int>(std::size(value.saEquipment))) return std::nullopt;
    for (int i = 0; i < message.equipment_size(); ++i)
        if (!decodeItem(message.equipment(i), value.saEquipment[static_cast<std::size_t>(i)])) return std::nullopt;
    normalizeVitals(value);
    return value;
}

std::optional<InventoryData> decodeInventory(const std::vector<uint8_t>& bytes) {
    if (bytes.empty() || bytes.size() > MaxBlob) return std::nullopt;
    const auto payload = unwrapState(bytes);
    if (!payload) return std::nullopt;
    zeropt::InventoryState message;
    if (!message.ParseFromArray(payload->data(), static_cast<int>(payload->size()))) return std::nullopt;
    InventoryData value{};
    if (!decodeItems(message, value.saItem, value.iGold)) return std::nullopt;
    return value;
}

std::optional<WarehouseData> decodeWarehouse(const std::vector<uint8_t>& bytes) {
    if (bytes.empty() || bytes.size() > MaxBlob) return std::nullopt;
    const auto payload = unwrapState(bytes);
    if (!payload) return std::nullopt;
    zeropt::WarehouseState message;
    if (!message.ParseFromArray(payload->data(), static_cast<int>(payload->size()))) return std::nullopt;
    WarehouseData value{};
    if (!decodeItems(message, value.saItem, value.iGold)) return std::nullopt;
    return value;
}
}
