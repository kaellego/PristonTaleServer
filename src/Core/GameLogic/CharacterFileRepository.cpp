#include "GameLogic/CharacterFileRepository.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <windows.h>

namespace fs = std::filesystem;

namespace {
#pragma pack(push, 1)
struct PackedItemV2 {
    int32_t sItemID;
    char szItemName[64];
    int32_t iQuantity;
    uint64_t serial;
    uint32_t checksum;
    uint32_t version;
    float fHPRegen, fMPRegen, fSPRegen;
};

struct PackedCharacterV2 {
    char szName[32];
    int32_t iLevel;
    int64_t liEXP;
    int32_t iClass;
    int32_t iMapId;
    float fPosX, fPosY, fPosZ;
    int32_t iHP, iMaxHP, iMP, iMaxMP;
    int32_t iSTM, iMaxSTM;
    float fHPRegen, fMPRegen, fSPRegen;
    float fHPRegenBonus, fMPRegenBonus, fSPRegenBonus;
    int32_t iStrength, iSpirit, iTalent, iAgility, iHealth;
    int32_t iAttackMin, iAttackMax, iDefense;
    int32_t iGold;
    PackedItemV2 saEquipment[16];
};
#pragma pack(pop)

static_assert(sizeof(PackedItemV2) == 100);
static_assert(sizeof(PackedCharacterV2) == 1748);
static_assert(sizeof(CharacterData) == sizeof(PackedCharacterV2));

struct LegacyItemV1 { int32_t id; char name[64]; };
struct LegacyCharacterV1 {
    char name[32]; int32_t level; int64_t experience; int32_t characterClass; int32_t mapId;
    float x, y, z; int32_t hp, maxHp, mp, maxMp;
    int32_t strength, spirit, talent, agility, health;
    int32_t attackMin, attackMax, defense, gold;
    LegacyItemV1 equipment[16];
};
struct LegacyCharacterPrefix {
    char name[32]; int32_t level; int64_t experience; int32_t characterClass; int32_t mapId;
    float x, y, z; int32_t hp, maxHp, mp, maxMp;
    int32_t strength, spirit, talent, agility, health;
    int32_t attackMin, attackMax, defense, gold;
};

void normalizeVitals(CharacterData& character) {
    if (character.iClass < 1 || character.iClass > 11) character.iClass = 1;
    character.iLevel = std::clamp(character.iLevel, 1, 150);

    if (character.iMaxHP <= 1) {
        character.iMaxHP = 100;
        if (character.iHP <= 0) character.iHP = character.iMaxHP;
    }
    character.iHP = std::clamp(character.iHP, 0, character.iMaxHP);

    if (character.iMaxMP <= 1) {
        character.iMaxMP = 100;
        if (character.iMP <= 0) character.iMP = character.iMaxMP;
    }
    character.iMP = std::clamp(character.iMP, 0, character.iMaxMP);

    if (character.iMaxSTM <= 1) {
        character.iMaxSTM = 100;
        if (character.iSTM <= 0) character.iSTM = character.iMaxSTM;
    }
    character.iSTM = std::clamp(character.iSTM, 0, character.iMaxSTM);
}

CharacterData convertLegacyPrefix(const LegacyCharacterPrefix& legacy) {
    CharacterData character{};
    std::memcpy(character.szName, legacy.name, sizeof(character.szName));
    character.iLevel = legacy.level;
    character.liEXP = std::max<int64_t>(0, legacy.experience);
    character.iClass = legacy.characterClass;
    character.iMapId = legacy.mapId;
    character.fPosX = legacy.x; character.fPosY = legacy.y; character.fPosZ = legacy.z;
    character.iHP = std::max(1, legacy.hp); character.iMaxHP = std::max(character.iHP, legacy.maxHp);
    character.iMP = std::max(0, legacy.mp); character.iMaxMP = std::max(character.iMP, legacy.maxMp);
    character.iStrength = std::max(1, legacy.strength); character.iSpirit = std::max(1, legacy.spirit);
    character.iTalent = std::max(1, legacy.talent); character.iAgility = std::max(1, legacy.agility);
    character.iHealth = std::max(1, legacy.health);
    character.iAttackMin = std::max(0, legacy.attackMin);
    character.iAttackMax = std::max(character.iAttackMin, legacy.attackMax);
    character.iDefense = std::max(0, legacy.defense);
    character.iGold = std::max(0, legacy.gold);
    normalizeVitals(character);
    return character;
}

Item convertPackedItem(const PackedItemV2& source) {
    Item item{};
    item.sItemID = source.sItemID;
    std::memcpy(item.szItemName, source.szItemName, sizeof(item.szItemName));
    item.iQuantity = source.iQuantity;
    item.serial = source.serial;
    item.checksum = source.checksum;
    item.version = source.version;
    item.fHPRegen = source.fHPRegen;
    item.fMPRegen = source.fMPRegen;
    item.fSPRegen = source.fSPRegen;
    return item;
}

PackedItemV2 toPackedItem(const Item& source) {
    PackedItemV2 item{};
    item.sItemID = source.sItemID;
    std::memcpy(item.szItemName, source.szItemName, sizeof(item.szItemName));
    item.iQuantity = source.iQuantity;
    item.serial = source.serial;
    item.checksum = source.checksum;
    item.version = source.version;
    item.fHPRegen = source.fHPRegen;
    item.fMPRegen = source.fMPRegen;
    item.fSPRegen = source.fSPRegen;
    return item;
}

CharacterData convertPackedCharacter(const PackedCharacterV2& source) {
    CharacterData character{};
    std::memcpy(character.szName, source.szName, sizeof(character.szName));
    character.iLevel = source.iLevel;
    character.liEXP = source.liEXP;
    character.iClass = source.iClass;
    character.iMapId = source.iMapId;
    character.fPosX = source.fPosX; character.fPosY = source.fPosY; character.fPosZ = source.fPosZ;
    character.iHP = source.iHP; character.iMaxHP = source.iMaxHP;
    character.iMP = source.iMP; character.iMaxMP = source.iMaxMP;
    character.iSTM = source.iSTM; character.iMaxSTM = source.iMaxSTM;
    character.fHPRegen = source.fHPRegen; character.fMPRegen = source.fMPRegen; character.fSPRegen = source.fSPRegen;
    character.fHPRegenBonus = source.fHPRegenBonus;
    character.fMPRegenBonus = source.fMPRegenBonus;
    character.fSPRegenBonus = source.fSPRegenBonus;
    character.iStrength = source.iStrength; character.iSpirit = source.iSpirit;
    character.iTalent = source.iTalent; character.iAgility = source.iAgility; character.iHealth = source.iHealth;
    character.iAttackMin = source.iAttackMin; character.iAttackMax = source.iAttackMax;
    character.iDefense = source.iDefense; character.iGold = source.iGold;
    for (size_t i = 0; i < std::size(character.saEquipment); ++i)
        character.saEquipment[i] = convertPackedItem(source.saEquipment[i]);
    normalizeVitals(character);
    return character;
}

PackedCharacterV2 toPackedCharacter(const CharacterData& source) {
    PackedCharacterV2 character{};
    std::memcpy(character.szName, source.szName, sizeof(character.szName));
    character.iLevel = source.iLevel;
    character.liEXP = source.liEXP;
    character.iClass = source.iClass;
    character.iMapId = source.iMapId;
    character.fPosX = source.fPosX; character.fPosY = source.fPosY; character.fPosZ = source.fPosZ;
    character.iHP = source.iHP; character.iMaxHP = source.iMaxHP;
    character.iMP = source.iMP; character.iMaxMP = source.iMaxMP;
    character.iSTM = source.iSTM; character.iMaxSTM = source.iMaxSTM;
    character.fHPRegen = source.fHPRegen; character.fMPRegen = source.fMPRegen; character.fSPRegen = source.fSPRegen;
    character.fHPRegenBonus = source.fHPRegenBonus;
    character.fMPRegenBonus = source.fMPRegenBonus;
    character.fSPRegenBonus = source.fSPRegenBonus;
    character.iStrength = source.iStrength; character.iSpirit = source.iSpirit;
    character.iTalent = source.iTalent; character.iAgility = source.iAgility; character.iHealth = source.iHealth;
    character.iAttackMin = source.iAttackMin; character.iAttackMax = source.iAttackMax;
    character.iDefense = source.iDefense; character.iGold = source.iGold;
    for (size_t i = 0; i < std::size(character.saEquipment); ++i)
        character.saEquipment[i] = toPackedItem(source.saEquipment[i]);
    return character;
}

bool writeAtomically(const fs::path& destination, const void* data, size_t size) {
    const fs::path temporary = destination.string() + ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
        file.flush();
        if (!file.good()) return false;
    }
    return MoveFileExA(temporary.string().c_str(), destination.string().c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}
}

CharacterFileRepository::CharacterFileRepository(const std::string& dataPath)
    : m_characterPath((fs::path(dataPath) / "Character").string()) {
    fs::create_directories(m_characterPath);
    migrateLegacyCharacters();
    std::cout << "Inicializando repositorio de jogadores: " << m_characterPath << std::endl;
}

void CharacterFileRepository::migrateLegacyCharacters() {
    struct LegacyCharacterData { char szName[32]; int32_t iLevel; int64_t liEXP; int32_t iClass; };
    for (const auto& entry : fs::directory_iterator(m_characterPath)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".chr") continue;
        std::error_code error;
        const auto size = fs::file_size(entry.path(), error);
        if (error || size != sizeof(LegacyCharacterData)) continue;

        LegacyCharacterData legacy{};
        std::ifstream input(entry.path(), std::ios::binary);
        if (!input.read(reinterpret_cast<char*>(&legacy), sizeof(legacy))) continue;

        LegacyCharacterPrefix prefix{};
        std::memcpy(prefix.name, legacy.szName, sizeof(prefix.name));
        prefix.level = legacy.iLevel;
        prefix.experience = legacy.liEXP;
        prefix.characterClass = legacy.iClass;
        auto upgraded = convertLegacyPrefix(prefix);
        upgraded.iHP = upgraded.iMaxHP = 100;
        upgraded.iMP = upgraded.iMaxMP = 100;
        upgraded.iSTM = upgraded.iMaxSTM = 100;
        upgraded.iStrength = upgraded.iSpirit = upgraded.iTalent = upgraded.iAgility = upgraded.iHealth = 1;
        upgraded.iAttackMin = upgraded.iAttackMax = 10;
        upgraded.iDefense = 0;
        if (save(upgraded)) std::cout << "Personagem legado migrado: " << entry.path().filename().string() << std::endl;
    }
}

std::optional<CharacterData> CharacterFileRepository::load(const std::string& characterName) const {
    std::lock_guard lock(m_mutex);
    const fs::path path = fs::path(m_characterPath) / (characterName + ".chr");
    std::error_code error;
    if (!fs::is_regular_file(path, error) || error) return std::nullopt;

    const auto fileSize = fs::file_size(path, error);
    if (error) return std::nullopt;
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;

    if (fileSize == sizeof(LegacyCharacterV1)) {
        LegacyCharacterV1 legacy{};
        if (!file.read(reinterpret_cast<char*>(&legacy), sizeof(legacy))) return std::nullopt;
        LegacyCharacterPrefix prefix{};
        std::memcpy(prefix.name, legacy.name, sizeof(prefix.name));
        prefix.level = legacy.level; prefix.experience = legacy.experience;
        prefix.characterClass = legacy.characterClass; prefix.mapId = legacy.mapId;
        prefix.x = legacy.x; prefix.y = legacy.y; prefix.z = legacy.z;
        prefix.hp = legacy.hp; prefix.maxHp = legacy.maxHp; prefix.mp = legacy.mp; prefix.maxMp = legacy.maxMp;
        prefix.strength = legacy.strength; prefix.spirit = legacy.spirit; prefix.talent = legacy.talent;
        prefix.agility = legacy.agility; prefix.health = legacy.health; prefix.attackMin = legacy.attackMin;
        prefix.attackMax = legacy.attackMax; prefix.defense = legacy.defense; prefix.gold = legacy.gold;
        auto character = convertLegacyPrefix(prefix);
        for (size_t i = 0; i < std::size(character.saEquipment); ++i) {
            character.saEquipment[i].sItemID = legacy.equipment[i].id;
            std::memcpy(character.saEquipment[i].szItemName, legacy.equipment[i].name,
                        sizeof(character.saEquipment[i].szItemName));
            character.saEquipment[i].iQuantity = legacy.equipment[i].id > 0 ? 1 : 0;
        }
        return character;
    }

    if (fileSize == sizeof(PackedCharacterV2)) {
        PackedCharacterV2 packed{};
        if (!file.read(reinterpret_cast<char*>(&packed), sizeof(packed))) return std::nullopt;
        return convertPackedCharacter(packed);
    }

    if (fileSize == 1204 || fileSize == 1268) {
        LegacyCharacterPrefix legacy{};
        if (!file.read(reinterpret_cast<char*>(&legacy), sizeof(legacy))) return std::nullopt;
        return convertLegacyPrefix(legacy);
    }

    return std::nullopt;
}

bool CharacterFileRepository::exists(const std::string& characterName) const {
    std::lock_guard lock(m_mutex);
    return fs::is_regular_file(fs::path(m_characterPath) / (characterName + ".chr"));
}

bool CharacterFileRepository::save(const CharacterData& character) const {
    std::lock_guard lock(m_mutex);
    fs::create_directories(m_characterPath);
    auto normalized = character;
    normalizeVitals(normalized);
    auto packed = toPackedCharacter(normalized);
    const fs::path path = fs::path(m_characterPath) / (std::string(normalized.szName) + ".chr");
    return writeAtomically(path, &packed, sizeof(packed));
}

bool CharacterFileRepository::remove(const std::string& characterName) const {
    std::lock_guard lock(m_mutex);
    return fs::remove(fs::path(m_characterPath) / (characterName + ".chr"));
}

std::vector<std::string> CharacterFileRepository::names() const {
    std::lock_guard lock(m_mutex);
    std::vector<std::string> result;
    std::error_code error;
    if (!fs::is_directory(m_characterPath, error) || error) return result;
    for (const auto& entry : fs::directory_iterator(m_characterPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".chr")
            result.push_back(entry.path().stem().string());
    }
    return result;
}
