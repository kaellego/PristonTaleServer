#include "GameLogic/PlayerRepository.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <windows.h>

namespace fs = std::filesystem;

namespace
{
    struct LegacyItemV1 { int32_t id; char name[64]; };
    struct LegacyCharacterV1 {
        char name[32]; int32_t level; int64_t experience; int32_t characterClass; int32_t mapId;
        float x, y, z; int32_t hp, maxHp, mp, maxMp;
        int32_t strength, spirit, talent, agility, health;
        int32_t attackMin, attackMax, defense, gold;
        LegacyItemV1 equipment[16];
    };
    struct LegacyWarehouseV1 { int32_t gold; LegacyItemV1 items[405]; };
    struct LegacyInventoryV1 { int32_t gold; LegacyItemV1 items[PLAYER_INVENTORY_SLOTS]; };

    Item convertItem(const LegacyItemV1& oldItem)
    {
        Item item{};
        item.sItemID = oldItem.id;
        std::memcpy(item.szItemName, oldItem.name, sizeof(item.szItemName));
        item.iQuantity = oldItem.id > 0 ? 1 : 0;
        return item;
    }

    bool writeAtomically(const fs::path& destination, const void* data, size_t size)
    {
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

PlayerRepository::PlayerRepository(const std::string& dataPath) : m_basePath(dataPath) {
    m_charPath = (fs::path(m_basePath) / "Character").string();
    m_warehousePath = (fs::path(m_basePath) / "Warehouse").string();
    fs::create_directories(m_charPath);
    fs::create_directories((fs::path(m_basePath) / "Inventory"));
    migrateLegacyCharacters();
    std::cout << "Inicializando repositorio de jogadores..." << std::endl;
}

void PlayerRepository::migrateLegacyCharacters() {
    struct LegacyCharacterData { char szName[32]; int32_t iLevel; int64_t liEXP; int32_t iClass; };
    for (const auto& entry : fs::directory_iterator(m_charPath)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".chr") continue;
        std::error_code error;
        const auto size = fs::file_size(entry.path(), error);
        if (error || size != sizeof(LegacyCharacterData)) continue;
        LegacyCharacterData legacy{};
        std::ifstream input(entry.path(), std::ios::binary);
        if (!input.read(reinterpret_cast<char*>(&legacy), sizeof(legacy))) continue;
        CharacterData upgraded{};
        std::memcpy(upgraded.szName, legacy.szName, sizeof(upgraded.szName));
        upgraded.iLevel = std::max(1, legacy.iLevel);
        upgraded.liEXP = std::max<int64_t>(0, legacy.liEXP);
        upgraded.iClass = std::clamp(legacy.iClass, 1, 11);
        upgraded.iHP = upgraded.iMaxHP = 100;
        upgraded.iMP = upgraded.iMaxMP = 100;
        upgraded.iStrength = upgraded.iSpirit = upgraded.iTalent = upgraded.iAgility = upgraded.iHealth = 1;
        upgraded.iAttackMin = upgraded.iAttackMax = 10;
        upgraded.iDefense = 0;
        if (saveCharacter(upgraded)) std::cout << "Personagem legado migrado: " << entry.path().filename().string() << std::endl;
    }
}

std::optional<CharacterData> PlayerRepository::loadCharacter(const std::string& characterName) const {
    std::lock_guard lock(m_mutex);
    fs::path filePath = fs::path(m_charPath) / (characterName + ".chr");

    if (!fs::exists(filePath)) {
        return std::nullopt;
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    CharacterData charData{};
    if (size == static_cast<std::streamsize>(sizeof(LegacyCharacterV1))) {
        LegacyCharacterV1 legacy{};
        if (!file.read(reinterpret_cast<char*>(&legacy), sizeof(legacy))) return std::nullopt;
        std::memcpy(charData.szName, legacy.name, sizeof(charData.szName));
        charData.iLevel = legacy.level; charData.liEXP = legacy.experience; charData.iClass = legacy.characterClass;
        charData.iMapId = legacy.mapId; charData.fPosX = legacy.x; charData.fPosY = legacy.y; charData.fPosZ = legacy.z;
        charData.iHP = legacy.hp; charData.iMaxHP = legacy.maxHp; charData.iMP = legacy.mp; charData.iMaxMP = legacy.maxMp;
        charData.iStrength = legacy.strength; charData.iSpirit = legacy.spirit; charData.iTalent = legacy.talent;
        charData.iAgility = legacy.agility; charData.iHealth = legacy.health; charData.iAttackMin = legacy.attackMin;
        charData.iAttackMax = legacy.attackMax; charData.iDefense = legacy.defense; charData.iGold = legacy.gold;
        for (size_t i = 0; i < std::size(charData.saEquipment); ++i) charData.saEquipment[i] = convertItem(legacy.equipment[i]);
        return charData;
    }
    if (size != static_cast<std::streamsize>(sizeof(CharacterData))) return std::nullopt;
    if (file.read(reinterpret_cast<char*>(&charData), sizeof(CharacterData))) {
        return charData;
    }

    return std::nullopt;
}

bool PlayerRepository::saveCharacter(const CharacterData& characterData) const {
    std::lock_guard lock(m_mutex);
    fs::create_directories(m_charPath);
    fs::path filePath = fs::path(m_charPath) / (std::string(characterData.szName) + ".chr");
    return writeAtomically(filePath, &characterData, sizeof(CharacterData));
}

bool PlayerRepository::deleteCharacter(const std::string& characterName) const {
    std::lock_guard lock(m_mutex);
    return fs::remove(fs::path(m_charPath) / (characterName + ".chr"));
}

std::optional<WarehouseData> PlayerRepository::loadWarehouse(const std::string& accountName) const {
    std::lock_guard lock(m_mutex);
    fs::path filePath = fs::path(m_warehousePath) / (accountName + ".dat");

    if (!fs::exists(filePath)) {
        return std::nullopt;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }

    file.seekg(0, std::ios::end);
    const auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    WarehouseData warehouseData{};
    if (fileSize == static_cast<std::streamoff>(sizeof(LegacyWarehouseV1))) {
        LegacyWarehouseV1 legacy{};
        if (!file.read(reinterpret_cast<char*>(&legacy), sizeof(legacy))) return std::nullopt;
        warehouseData.iGold = legacy.gold;
        for (size_t i = 0; i < std::size(warehouseData.saItem); ++i) warehouseData.saItem[i] = convertItem(legacy.items[i]);
        return warehouseData;
    }
    if (fileSize != static_cast<std::streamoff>(sizeof(WarehouseData))) return std::nullopt;
    if (file.read(reinterpret_cast<char*>(&warehouseData), sizeof(WarehouseData))) {
        return warehouseData;
    }

    return std::nullopt;
}

bool PlayerRepository::saveWarehouse(const std::string& accountName, const WarehouseData& warehouseData) const {
    std::lock_guard lock(m_mutex);
    fs::create_directories(m_warehousePath);
    fs::path filePath = fs::path(m_warehousePath) / (accountName + ".dat");
    return writeAtomically(filePath, &warehouseData, sizeof(WarehouseData));
}

std::optional<InventoryData> PlayerRepository::loadInventory(const std::string& characterName) const {
    std::lock_guard lock(m_mutex);
    std::ifstream file(fs::path(m_basePath) / "Inventory" / (characterName + ".dat"), std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    file.seekg(0, std::ios::end);
    const auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    InventoryData data{};
    if (fileSize == static_cast<std::streamoff>(sizeof(LegacyInventoryV1))) {
        LegacyInventoryV1 legacy{};
        if (!file.read(reinterpret_cast<char*>(&legacy), sizeof(legacy))) return std::nullopt;
        data.iGold = legacy.gold;
        for (size_t i = 0; i < std::size(data.saItem); ++i) data.saItem[i] = convertItem(legacy.items[i]);
        return data;
    }
    if (fileSize != static_cast<std::streamoff>(sizeof(InventoryData))) return std::nullopt;
    return file.read(reinterpret_cast<char*>(&data), sizeof(data)) ? std::optional<InventoryData>(data) : std::nullopt;
}

bool PlayerRepository::saveInventory(const std::string& characterName, const InventoryData& inventoryData) const {
    std::lock_guard lock(m_mutex);
    fs::create_directories(fs::path(m_basePath) / "Inventory");
    return writeAtomically(fs::path(m_basePath) / "Inventory" / (characterName + ".dat"),
                           &inventoryData, sizeof(InventoryData));
}

std::vector<std::string> PlayerRepository::getAllCharacterNames() const {
    std::lock_guard lock(m_mutex);
    std::vector<std::string> names;
    if (!fs::exists(m_charPath) || !fs::is_directory(m_charPath)) {
        return names;
    }
    for (const auto& entry : fs::directory_iterator(m_charPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".chr") {
            names.push_back(entry.path().stem().string());
        }
    }
    return names;
}
