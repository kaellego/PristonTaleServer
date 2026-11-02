#include "GameLogic/StorageFileRepository.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <vector>
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

struct PackedWarehouseV2 { int32_t iGold; PackedItemV2 saItem[405]; };
struct PackedInventoryV2 { int32_t iGold; PackedItemV2 saItem[PLAYER_INVENTORY_SLOTS]; };
#pragma pack(pop)

static_assert(sizeof(PackedItemV2) == 100);
static_assert(sizeof(PackedWarehouseV2) == 40504);
static_assert(sizeof(PackedInventoryV2) == 6004);

struct LegacyItemV1 { int32_t id; char name[64]; };
struct LegacyWarehouseV1 { int32_t gold; LegacyItemV1 items[405]; };
struct LegacyInventoryV1 { int32_t gold; LegacyItemV1 items[PLAYER_INVENTORY_SLOTS]; };

static_assert(sizeof(LegacyWarehouseV1) == 405 * sizeof(LegacyItemV1) + sizeof(int32_t));
static_assert(sizeof(LegacyInventoryV1) == PLAYER_INVENTORY_SLOTS * sizeof(LegacyItemV1) + sizeof(int32_t));

Item convertItem(const LegacyItemV1& source) {
    Item item{};
    item.sItemID = source.id;
    std::memcpy(item.szItemName, source.name, sizeof(item.szItemName));
    item.iQuantity = source.id > 0 ? 1 : 0;
    return item;
}

Item convertItem(const PackedItemV2& source) {
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

template <typename T>
std::optional<T> readFile(const fs::path& path, size_t legacySize,
                          const std::function<void(const std::vector<uint8_t>&, T&)>& convert) {
    std::error_code error;
    if (!fs::is_regular_file(path, error) || error) return std::nullopt;
    const auto size = fs::file_size(path, error);
    if (error || (size != sizeof(T) && size != legacySize)) return std::nullopt;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) return std::nullopt;

    T value{};
    if (size == sizeof(T)) std::memcpy(&value, bytes.data(), sizeof(T));
    else convert(bytes, value);
    return value;
}
}

StorageFileRepository::StorageFileRepository(const std::string& dataPath)
    : m_warehousePath((fs::path(dataPath) / "Warehouse").string()),
      m_inventoryPath((fs::path(dataPath) / "Inventory").string()) {
    fs::create_directories(m_warehousePath);
    fs::create_directories(m_inventoryPath);
}

std::optional<WarehouseData> StorageFileRepository::loadWarehouse(const std::string& accountName) const {
    std::lock_guard lock(m_mutex);
    auto packed = readFile<PackedWarehouseV2>(fs::path(m_warehousePath) / (accountName + ".dat"),
                                   sizeof(LegacyWarehouseV1),
                                   [](const std::vector<uint8_t>& bytes, PackedWarehouseV2& target) {
                                       LegacyWarehouseV1 legacy{};
                                       std::memcpy(&legacy, bytes.data(), sizeof(legacy));
                                       target.iGold = legacy.gold;
                                       for (size_t i = 0; i < std::size(target.saItem); ++i)
                                           target.saItem[i] = toPackedItem(convertItem(legacy.items[i]));
                                   });
    if (!packed) return std::nullopt;
    WarehouseData result{};
    result.iGold = packed->iGold;
    for (size_t i = 0; i < std::size(result.saItem); ++i)
        result.saItem[i] = convertItem(packed->saItem[i]);
    return result;
}

bool StorageFileRepository::saveWarehouse(const std::string& accountName, const WarehouseData& warehouse) const {
    std::lock_guard lock(m_mutex);
    fs::create_directories(m_warehousePath);
    PackedWarehouseV2 packed{};
    packed.iGold = warehouse.iGold;
    for (size_t i = 0; i < std::size(packed.saItem); ++i)
        packed.saItem[i] = toPackedItem(warehouse.saItem[i]);
    return writeAtomically(fs::path(m_warehousePath) / (accountName + ".dat"), &packed, sizeof(packed));
}

bool StorageFileRepository::deleteWarehouse(const std::string& accountName) const {
    std::lock_guard lock(m_mutex);
    return fs::remove(fs::path(m_warehousePath) / (accountName + ".dat"));
}

std::optional<InventoryData> StorageFileRepository::loadInventory(const std::string& characterName) const {
    std::lock_guard lock(m_mutex);
    auto packed = readFile<PackedInventoryV2>(fs::path(m_inventoryPath) / (characterName + ".dat"),
                                   sizeof(LegacyInventoryV1),
                                   [](const std::vector<uint8_t>& bytes, PackedInventoryV2& target) {
                                       LegacyInventoryV1 legacy{};
                                       std::memcpy(&legacy, bytes.data(), sizeof(legacy));
                                       target.iGold = legacy.gold;
                                       for (size_t i = 0; i < std::size(target.saItem); ++i)
                                           target.saItem[i] = toPackedItem(convertItem(legacy.items[i]));
                                   });
    if (!packed) return std::nullopt;
    InventoryData result{};
    result.iGold = packed->iGold;
    for (size_t i = 0; i < std::size(result.saItem); ++i)
        result.saItem[i] = convertItem(packed->saItem[i]);
    return result;
}

bool StorageFileRepository::saveInventory(const std::string& characterName, const InventoryData& inventory) const {
    std::lock_guard lock(m_mutex);
    fs::create_directories(m_inventoryPath);
    PackedInventoryV2 packed{};
    packed.iGold = inventory.iGold;
    for (size_t i = 0; i < std::size(packed.saItem); ++i)
        packed.saItem[i] = toPackedItem(inventory.saItem[i]);
    return writeAtomically(fs::path(m_inventoryPath) / (characterName + ".dat"), &packed, sizeof(packed));
}

bool StorageFileRepository::deleteInventory(const std::string& characterName) const {
    std::lock_guard lock(m_mutex);
    return fs::remove(fs::path(m_inventoryPath) / (characterName + ".dat"));
}
