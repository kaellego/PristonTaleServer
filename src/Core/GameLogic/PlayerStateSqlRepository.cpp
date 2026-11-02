#include "GameLogic/PlayerStateSqlRepository.h"

#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "GameLogic/ItemIntegrity.h"
#include "GameLogic/PlayerStateProto.h"
#include "Utils/HmacSha256.h"

#include <array>
#include <cctype>
#include <exception>
#include <stdexcept>

namespace {
constexpr char Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::optional<std::vector<uint8_t>> decodeBase64(const std::string& text) {
    if (text.empty() || text.size() % 4 != 0) return std::nullopt;
    std::array<int, 256> lookup{};
    lookup.fill(-1);
    for (int i = 0; Base64[i] != '\0'; ++i) lookup[static_cast<unsigned char>(Base64[i])] = i;
    std::vector<uint8_t> result;
    result.reserve((text.size() / 4) * 3);
    for (std::size_t i = 0; i < text.size(); i += 4) {
        const auto a = lookup[static_cast<unsigned char>(text[i])];
        const auto b = lookup[static_cast<unsigned char>(text[i + 1])];
        if (a < 0 || b < 0) return std::nullopt;
        const auto c = text[i + 2] == '=' ? 0 : lookup[static_cast<unsigned char>(text[i + 2])];
        const auto d = text[i + 3] == '=' ? 0 : lookup[static_cast<unsigned char>(text[i + 3])];
        if (c < 0 || d < 0) return std::nullopt;
        const uint32_t value = (static_cast<uint32_t>(a) << 18) | (static_cast<uint32_t>(b) << 12) |
                               (static_cast<uint32_t>(c) << 6) | static_cast<uint32_t>(d);
        result.push_back(static_cast<uint8_t>(value >> 16));
        if (text[i + 2] != '=') result.push_back(static_cast<uint8_t>(value >> 8));
        if (text[i + 3] != '=') result.push_back(static_cast<uint8_t>(value));
    }
    return result;
}

std::string sqlOwner(const std::string& owner) {
    if (owner.empty() || owner.size() > 32) throw std::invalid_argument("owner de estado invalido");
    for (const auto character : owner)
        if (!std::isalnum(static_cast<unsigned char>(character)) && character != '_' && character != '-')
            throw std::invalid_argument("owner de estado invalido");
    return owner;
}

std::optional<std::array<uint8_t, 32>> stateMac(int type, const std::string& owner, const std::vector<uint8_t>& bytes) {
    std::vector<uint8_t> authenticated;
    authenticated.reserve(sizeof(type) + owner.size() + 1 + bytes.size());
    for (std::size_t i = 0; i < sizeof(type); ++i) authenticated.push_back(static_cast<uint8_t>(type >> (i * 8)));
    authenticated.insert(authenticated.end(), owner.begin(), owner.end());
    authenticated.push_back(0);
    authenticated.insert(authenticated.end(), bytes.begin(), bytes.end());
    return HmacSha256::calculate(ItemIntegrity::secret(), authenticated);
}

template <typename T>
bool validateItems(T& items) {
    for (auto& item : items) {
        if (item.sItemID <= 0) continue;
        if (item.iQuantity <= 0 || (item.serial != 0 && item.checksum != 0 && item.checksum != ItemIntegrity::checksum(item)))
            return false;
        ItemIntegrity::initialize(item);
    }
    return true;
}

template <typename T>
void normalizeItems(T& items) {
    for (auto& item : items) ItemIntegrity::initialize(item);
}
}

std::optional<PlayerStateSqlRepository::LoadedBlob> PlayerStateSqlRepository::loadBlob(int type, const std::string& owner) const {
    try {
        auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary);
        const auto key = sqlOwner(owner);
        db->prepare("SELECT ChunkData, StateVersion, StateMac FROM dbo.PlayerStateProto WHERE StateType=? AND OwnerKey=? ORDER BY ChunkIndex");
        db->bindParameter(1, type);
        db->bindParameter(2, key);
        db->execute();
        std::vector<uint8_t> legacyEncoded;
        std::vector<uint8_t> state;
        int stateVersion = 0;
        bool hasLegacyChunks = false;
        bool hasBinaryState = false;
        while (db->fetch()) {
            const auto chunk = db->getData<std::vector<uint8_t>>(1);
            const auto version = db->getData<int>(2);
            if (!chunk || !version || chunk->size() > 1024 * 1024) return std::nullopt;
            stateVersion = *version;
            if (stateVersion >= 2) {
                if (hasLegacyChunks) return std::nullopt;
                hasBinaryState = true;
                if (!state.empty()) return std::nullopt;
                const auto mac = db->getData<std::vector<uint8_t>>(3);
                const auto expectedMac = stateMac(type, key, *chunk);
                if (!mac || mac->size() != 32 || !expectedMac || !HmacSha256::constantTimeEqual(*mac, *expectedMac))
                    return std::nullopt;
                state = *chunk;
            } else {
                db->getData<std::vector<uint8_t>>(3);
                if (hasBinaryState) return std::nullopt;
                hasLegacyChunks = true;
                legacyEncoded.insert(legacyEncoded.end(), chunk->begin(), chunk->end());
                if (legacyEncoded.size() > 1024 * 1024) return std::nullopt;
            }
        }
        if (stateVersion >= 2)
            return state.empty() ? std::nullopt : std::optional<LoadedBlob>(LoadedBlob{std::move(state), stateVersion});
        auto decoded = decodeBase64(std::string(legacyEncoded.begin(), legacyEncoded.end()));
        return decoded ? std::optional<LoadedBlob>(LoadedBlob{std::move(*decoded), 1}) : std::nullopt;
    } catch (...) { return std::nullopt; }
}

bool PlayerStateSqlRepository::hasBlob(int type, const std::string& owner) const {
    try {
        auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary);
        const auto key = sqlOwner(owner);
        db->prepare("SELECT TOP 1 ChunkIndex FROM dbo.PlayerStateProto WHERE StateType=? AND OwnerKey=?");
        db->bindParameter(1, type);
        db->bindParameter(2, key);
        db->execute();
        return db->fetch();
    } catch (...) { return false; }
}

bool PlayerStateSqlRepository::saveBlob(SQLConnection& db, int type, const std::string& owner, const std::vector<uint8_t>& bytes) const {
    if (bytes.empty() || bytes.size() > 1024 * 1024) return false;
    const auto key = sqlOwner(owner);
    db.prepare("DELETE FROM dbo.PlayerStateProto WHERE StateType=? AND OwnerKey=?");
    db.bindParameter(1, type);
    db.bindParameter(2, key);
    db.execute();
    const auto mac = stateMac(type, key, bytes);
    if (!mac) return false;
    db.prepare("INSERT INTO dbo.PlayerStateProto (StateType,OwnerKey,ChunkIndex,ChunkData,StateVersion,StateMac) VALUES (?, ?, 0, ?, 2, ?)");
    db.bindParameter(1, type);
    db.bindParameter(2, key);
    db.bindParameter(3, bytes);
    const std::vector<uint8_t> macBytes(mac->begin(), mac->end());
    db.bindParameter(4, macBytes);
    db.execute();
    return true;
}

bool PlayerStateSqlRepository::deleteBlob(int type, const std::string& owner) const {
    try {
        auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary);
        const auto key = sqlOwner(owner);
        db->prepare("DELETE FROM dbo.PlayerStateProto WHERE StateType=? AND OwnerKey=?");
        db->bindParameter(1, type);
        db->bindParameter(2, key);
        db->execute();
        return true;
    } catch (...) { return false; }
}

std::optional<CharacterData> PlayerStateSqlRepository::loadCharacter(const std::string& name) const {
    const auto blob = loadBlob(1, name);
    if (!blob) return std::nullopt;
    auto value = PlayerStateProto::decodeCharacter(blob->bytes);
    if (value && blob->version < 2) saveCharacter(*value);
    return value;
}

bool PlayerStateSqlRepository::hasCharacterState(const std::string& name) const { return hasBlob(1, name); }

bool PlayerStateSqlRepository::saveCharacter(const CharacterData& value) const {
    try {
        auto normalized = value;
        normalizeItems(normalized.saEquipment);
        auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->beginTransaction();
        if (!saveBlob(*db, 1, std::string(normalized.szName), PlayerStateProto::encode(normalized))) { db->rollback(); return false; }
        const auto key = sqlOwner(std::string(normalized.szName));
        db->prepare("UPDATE dbo.CharacterInfo SET Experience=" + std::to_string(normalized.liEXP) + ", StateUpdatedAt=sysdatetime() WHERE Name='" + key + "'");
        db->execute();
        db->commit(); return true;
    } catch (...) { return false; }
}

bool PlayerStateSqlRepository::deleteCharacter(const std::string& name) const {
    return deleteBlob(1, name) && deleteBlob(2, name);
}

std::optional<InventoryData> PlayerStateSqlRepository::loadInventory(const std::string& character) const {
    const auto blob = loadBlob(2, character);
    if (!blob) return std::nullopt;
    auto result = PlayerStateProto::decodeInventory(blob->bytes);
    if (!result || !validateItems(result->saItem)) return std::nullopt;
    if (blob->version < 2) saveInventory(character, *result);
    return result;
}

bool PlayerStateSqlRepository::hasInventoryState(const std::string& character) const {
    return hasBlob(2, character);
}

bool PlayerStateSqlRepository::saveInventory(const std::string& character, const InventoryData& value) const {
    try { auto normalized = value; normalizeItems(normalized.saItem); auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->beginTransaction(); const bool ok = saveBlob(*db, 2, character, PlayerStateProto::encode(normalized)); if (ok) db->commit(); else db->rollback(); return ok; } catch (...) { return false; }
}

bool PlayerStateSqlRepository::deleteInventory(const std::string& character) const { return deleteBlob(2, character); }

std::optional<WarehouseData> PlayerStateSqlRepository::loadWarehouse(const std::string& account) const {
    const auto blob = loadBlob(3, account);
    if (!blob) return std::nullopt;
    auto result = PlayerStateProto::decodeWarehouse(blob->bytes);
    if (!result || !validateItems(result->saItem)) return std::nullopt;
    if (blob->version < 2) saveWarehouse(account, *result);
    return result;
}

bool PlayerStateSqlRepository::hasWarehouseState(const std::string& account) const {
    return hasBlob(3, account);
}

bool PlayerStateSqlRepository::saveWarehouse(const std::string& account, const WarehouseData& value) const {
    try { auto normalized = value; normalizeItems(normalized.saItem); auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->beginTransaction(); const bool ok = saveBlob(*db, 3, account, PlayerStateProto::encode(normalized)); if (ok) db->commit(); else db->rollback(); return ok; } catch (...) { return false; }
}

bool PlayerStateSqlRepository::deleteWarehouse(const std::string& account) const { return deleteBlob(3, account); }

bool PlayerStateSqlRepository::saveInventoryAndWarehouse(const std::string& character, const InventoryData& inventory,
                                                         const std::string& account, const WarehouseData& warehouse) const {
    try {
        auto normalizedInventory = inventory;
        auto normalizedWarehouse = warehouse;
        normalizeItems(normalizedInventory.saItem);
        normalizeItems(normalizedWarehouse.saItem);
        auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->beginTransaction();
        const bool ok = saveBlob(*db, 2, character, PlayerStateProto::encode(normalizedInventory)) && saveBlob(*db, 3, account, PlayerStateProto::encode(normalizedWarehouse));
        if (ok) db->commit(); else db->rollback();
        return ok;
    } catch (...) { return false; }
}

bool PlayerStateSqlRepository::saveCharacterAndInventory(const CharacterData& character, const InventoryData& inventory) const {
    try {
        auto normalizedCharacter = character;
        auto normalizedInventory = inventory;
        normalizeItems(normalizedCharacter.saEquipment);
        normalizeItems(normalizedInventory.saItem);
        auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary);
        db->beginTransaction();
        const auto name = sqlOwner(std::string(normalizedCharacter.szName));
        const bool ok = saveBlob(*db, 1, name, PlayerStateProto::encode(normalizedCharacter)) &&
                        saveBlob(*db, 2, name, PlayerStateProto::encode(normalizedInventory));
        if (ok) {
            db->prepare("UPDATE dbo.CharacterInfo SET Experience=" + std::to_string(normalizedCharacter.liEXP) + ", StateUpdatedAt=sysdatetime() WHERE Name='" + name + "'");
            db->execute();
            db->commit();
        } else db->rollback();
        return ok;
    } catch (...) { return false; }
}
