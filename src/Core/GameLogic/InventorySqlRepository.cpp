#include "GameLogic/InventorySqlRepository.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "GameLogic/ItemIntegrity.h"
#include <algorithm>
#include <cstring>

namespace {
    void copyName(char (&destination)[64], const std::string& source) {
        std::memset(destination, 0, sizeof(destination));
        std::memcpy(destination, source.data(), std::min(source.size(), sizeof(destination) - 1));
    }
}

std::optional<InventoryData> InventorySqlRepository::loadInventory(const std::string& character) const {
    try {
        auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary);
        db->prepare("SELECT ci.SlotIndex, ci.ItemCode, ci.ItemName, ci.Quantity, ci.ItemSerial, ci.ItemChecksum, ci.ItemVersion FROM dbo.CharacterInventory ci WHERE ci.CharacterName=? AND EXISTS (SELECT 1 FROM GameDB.dbo.ItemList i WHERE i.ItemCode=ci.ItemCode) ORDER BY ci.SlotIndex");
        db->bindParameter(1, character); db->execute();
        InventoryData result{};
        while (db->fetch()) {
            const auto slot = db->getData<int32_t>(1); const auto code = db->getData<int32_t>(2);
            const auto name = db->getData<std::string>(3); const auto quantity = db->getData<int32_t>(4); const auto serial = db->getData<uint64_t>(5); const auto checksum = db->getData<uint32_t>(6); const auto version = db->getData<uint32_t>(7);
            if (!slot || !code || !quantity || *slot < 0 || *slot >= static_cast<int32_t>(PLAYER_INVENTORY_SLOTS) || *code <= 0 || *quantity <= 0) continue;
            result.saItem[*slot].sItemID = *code; result.saItem[*slot].iQuantity = *quantity;
            if (name) copyName(result.saItem[*slot].szItemName, *name);
            result.saItem[*slot].serial = serial.value_or(0); result.saItem[*slot].checksum = checksum.value_or(0); result.saItem[*slot].version = version.value_or(0);
            if (result.saItem[*slot].serial != 0 && result.saItem[*slot].checksum != 0 && result.saItem[*slot].checksum != ItemIntegrity::checksum(result.saItem[*slot])) continue;
            ItemIntegrity::initialize(result.saItem[*slot]);
        }
        return result;
    } catch (...) { return std::nullopt; }
}

std::optional<WarehouseData> InventorySqlRepository::loadWarehouse(const std::string& account) const {
    try {
        auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary);
        db->prepare("SELECT aw.SlotIndex, aw.ItemCode, aw.ItemName, aw.Quantity, aw.ItemSerial, aw.ItemChecksum, aw.ItemVersion FROM dbo.AccountWarehouse aw WHERE aw.AccountName=? AND EXISTS (SELECT 1 FROM GameDB.dbo.ItemList i WHERE i.ItemCode=aw.ItemCode) ORDER BY aw.SlotIndex");
        db->bindParameter(1, account); db->execute();
        WarehouseData result{};
        while (db->fetch()) {
            const auto slot = db->getData<int32_t>(1); const auto code = db->getData<int32_t>(2);
            const auto name = db->getData<std::string>(3); const auto quantity = db->getData<int32_t>(4); const auto serial = db->getData<uint64_t>(5); const auto checksum = db->getData<uint32_t>(6); const auto version = db->getData<uint32_t>(7);
            if (!slot || !code || !quantity || *slot < 0 || *slot >= 405 || *code <= 0 || *quantity <= 0) continue;
            result.saItem[*slot].sItemID = *code; result.saItem[*slot].iQuantity = *quantity;
            if (name) copyName(result.saItem[*slot].szItemName, *name);
            result.saItem[*slot].serial = serial.value_or(0); result.saItem[*slot].checksum = checksum.value_or(0); result.saItem[*slot].version = version.value_or(0);
            if (result.saItem[*slot].serial != 0 && result.saItem[*slot].checksum != 0 && result.saItem[*slot].checksum != ItemIntegrity::checksum(result.saItem[*slot])) continue;
            ItemIntegrity::initialize(result.saItem[*slot]);
        }
        return result;
    } catch (...) { return std::nullopt; }
}

bool InventorySqlRepository::saveInventory(const std::string& character, const InventoryData& data) const {
    std::unique_ptr<SQLConnection> db;
    try {
        db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->beginTransaction();
        db->prepare("DELETE FROM dbo.CharacterInventory WHERE CharacterName=?"); db->bindParameter(1, character); db->execute();
        for (int32_t slot = 0; slot < static_cast<int32_t>(PLAYER_INVENTORY_SLOTS); ++slot) {
            Item item = data.saItem[slot]; ItemIntegrity::initialize(item); if (item.sItemID <= 0) continue;
            const int32_t quantity = item.iQuantity > 0 ? item.iQuantity : 1;
            db->prepare("INSERT INTO dbo.CharacterInventory (CharacterName,SlotIndex,ItemCode,ItemName,Quantity,ItemSerial,ItemChecksum,ItemVersion) VALUES (?,?,?,?,?,?,?,?)");
            db->bindParameter(1, character); db->bindParameter(2, slot); db->bindParameter(3, item.sItemID);
            db->bindParameter(4, std::string(item.szItemName)); db->bindParameter(5, quantity); db->bindParameter(6, item.serial); db->bindParameter(7, item.checksum); db->bindParameter(8, item.version); db->execute();
        }
        db->commit(); return true;
    } catch (...) { if (db) { try { db->rollback(); } catch (...) {} } return false; }
}

bool InventorySqlRepository::saveWarehouse(const std::string& account, const WarehouseData& data) const {
    std::unique_ptr<SQLConnection> db;
    try {
        db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->beginTransaction();
        db->prepare("DELETE FROM dbo.AccountWarehouse WHERE AccountName=?"); db->bindParameter(1, account); db->execute();
        for (int32_t slot = 0; slot < 405; ++slot) {
            Item item = data.saItem[slot]; ItemIntegrity::initialize(item); if (item.sItemID <= 0) continue;
            const int32_t quantity = item.iQuantity > 0 ? item.iQuantity : 1;
            db->prepare("INSERT INTO dbo.AccountWarehouse (AccountName,SlotIndex,ItemCode,ItemName,Quantity,ItemSerial,ItemChecksum,ItemVersion) VALUES (?,?,?,?,?,?,?,?)");
            db->bindParameter(1, account); db->bindParameter(2, slot); db->bindParameter(3, item.sItemID);
            db->bindParameter(4, std::string(item.szItemName)); db->bindParameter(5, quantity); db->bindParameter(6, item.serial); db->bindParameter(7, item.checksum); db->bindParameter(8, item.version); db->execute();
        }
        db->commit(); return true;
    } catch (...) { if (db) { try { db->rollback(); } catch (...) {} } return false; }
}

bool InventorySqlRepository::saveInventoryAndWarehouse(const std::string& character, const InventoryData& inventory, const std::string& account, const WarehouseData& warehouse) const {
    std::unique_ptr<SQLConnection> db;
    try {
        db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->beginTransaction();
        db->prepare("DELETE FROM dbo.CharacterInventory WHERE CharacterName=?"); db->bindParameter(1, character); db->execute();
        for (int32_t slot = 0; slot < static_cast<int32_t>(PLAYER_INVENTORY_SLOTS); ++slot) {
            Item item = inventory.saItem[slot]; ItemIntegrity::initialize(item); if (item.sItemID <= 0) continue;
            const int32_t quantity = item.iQuantity > 0 ? item.iQuantity : 1;
            db->prepare("INSERT INTO dbo.CharacterInventory (CharacterName,SlotIndex,ItemCode,ItemName,Quantity,ItemSerial,ItemChecksum,ItemVersion) VALUES (?,?,?,?,?,?,?,?)");
            db->bindParameter(1, character); db->bindParameter(2, slot); db->bindParameter(3, item.sItemID); db->bindParameter(4, std::string(item.szItemName)); db->bindParameter(5, quantity); db->bindParameter(6, item.serial); db->bindParameter(7, item.checksum); db->bindParameter(8, item.version); db->execute();
        }
        db->prepare("DELETE FROM dbo.AccountWarehouse WHERE AccountName=?"); db->bindParameter(1, account); db->execute();
        for (int32_t slot = 0; slot < 405; ++slot) {
            Item item = warehouse.saItem[slot]; ItemIntegrity::initialize(item); if (item.sItemID <= 0) continue;
            const int32_t quantity = item.iQuantity > 0 ? item.iQuantity : 1;
            db->prepare("INSERT INTO dbo.AccountWarehouse (AccountName,SlotIndex,ItemCode,ItemName,Quantity,ItemSerial,ItemChecksum,ItemVersion) VALUES (?,?,?,?,?,?,?,?)");
            db->bindParameter(1, account); db->bindParameter(2, slot); db->bindParameter(3, item.sItemID); db->bindParameter(4, std::string(item.szItemName)); db->bindParameter(5, quantity); db->bindParameter(6, item.serial); db->bindParameter(7, item.checksum); db->bindParameter(8, item.version); db->execute();
        }
        db->commit(); return true;
    } catch (...) { if (db) { try { db->rollback(); } catch (...) {} } return false; }
}
