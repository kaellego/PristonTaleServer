#include "GameLogic/PlayerRepository.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

PlayerRepository::PlayerRepository(const std::string& dataPath) : m_basePath(dataPath) {
    m_charPath = (fs::path(m_basePath) / "Character").string();
    m_warehousePath = (fs::path(m_basePath) / "Warehouse").string();
    fs::create_directories((fs::path(m_basePath) / "Inventory"));
    std::cout << "Inicializando repositorio de jogadores..." << std::endl;
}

std::optional<CharacterData> PlayerRepository::loadCharacter(const std::string& characterName) const {
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

    if (size != sizeof(CharacterData)) {
        return std::nullopt;
    }

    CharacterData charData;
    if (file.read(reinterpret_cast<char*>(&charData), sizeof(CharacterData))) {
        return charData;
    }

    return std::nullopt;
}

bool PlayerRepository::saveCharacter(const CharacterData& characterData) const {
    fs::create_directories(m_charPath);
    fs::path filePath = fs::path(m_charPath) / (std::string(characterData.szName) + ".chr");
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(&characterData), sizeof(CharacterData));
    return file.good();
}

bool PlayerRepository::deleteCharacter(const std::string& characterName) const {
    return fs::remove(fs::path(m_charPath) / (characterName + ".chr"));
}

std::optional<WarehouseData> PlayerRepository::loadWarehouse(const std::string& accountName) const {
    fs::path filePath = fs::path(m_warehousePath) / (accountName + ".dat");

    if (!fs::exists(filePath)) {
        return std::nullopt;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }

    WarehouseData warehouseData;
    if (file.read(reinterpret_cast<char*>(&warehouseData), sizeof(WarehouseData))) {
        return warehouseData;
    }

    return std::nullopt;
}

bool PlayerRepository::saveWarehouse(const std::string& accountName, const WarehouseData& warehouseData) const {
    fs::create_directories(m_warehousePath);
    fs::path filePath = fs::path(m_warehousePath) / (accountName + ".dat");
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(&warehouseData), sizeof(WarehouseData));
    return file.good();
}

std::optional<InventoryData> PlayerRepository::loadInventory(const std::string& characterName) const {
    std::ifstream file(fs::path(m_basePath) / "Inventory" / (characterName + ".dat"), std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    InventoryData data{};
    return file.read(reinterpret_cast<char*>(&data), sizeof(data)) ? std::optional<InventoryData>(data) : std::nullopt;
}

bool PlayerRepository::saveInventory(const std::string& characterName, const InventoryData& inventoryData) const {
    fs::create_directories(fs::path(m_basePath) / "Inventory");
    std::ofstream file(fs::path(m_basePath) / "Inventory" / (characterName + ".dat"), std::ios::binary | std::ios::trunc);
    return file.is_open() && static_cast<bool>(file.write(reinterpret_cast<const char*>(&inventoryData), sizeof(inventoryData)));
}

std::vector<std::string> PlayerRepository::getAllCharacterNames() const {
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
