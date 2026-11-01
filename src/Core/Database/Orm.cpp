#include "Database/Orm.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "Logging/LogService.h"

Orm::Orm(DatabasePool& pool, LogService& log, DatabaseDialect dialect)
    : m_pool(pool), m_log(log), m_dialect(dialect) {}

std::optional<SQLUser> Orm::findUser(const std::string& accountName) const {
    auto db = m_pool.getConnection(EDatabaseID::UserDB_Primary);
    if (!db) return std::nullopt;
    const auto limit = selectLimit(m_dialect, 1);
    db->prepare("SELECT" + limit + " ID, AccountName, Password, Flag, BanStatus, IsMuted FROM UserInfo WHERE AccountName=?");
    db->bindParameter<std::string>(1, accountName);
    db->execute();
    if (!db->fetch()) return std::nullopt;

    SQLUser user{};
    user.iID = db->getData<int>(1).value_or(0);
    const auto name = db->getData<std::string>(2).value_or("");
    const auto password = db->getData<std::string>(3).value_or("");
    strncpy_s(user.szAccountName, sizeof(user.szAccountName), name.c_str(), _TRUNCATE);
    strncpy_s(user.szPassword, sizeof(user.szPassword), password.c_str(), _TRUNCATE);
    user.iFlag = db->getData<int>(4).value_or(0);
    user.iBanStatus = static_cast<EBanStatus>(db->getData<int>(5).value_or(0));
    user.bIsMuted = db->getData<bool>(6).value_or(false);
    return user;
}

std::vector<std::string> Orm::findCharacterNames(const std::string& accountName, int limit) const {
    std::vector<std::string> names;
    auto db = m_pool.getConnection(EDatabaseID::UserDB_Primary);
    if (!db) return names;
    const auto top = selectLimit(m_dialect, limit);
    const auto suffix = queryLimit(m_dialect, limit);
    db->prepare("SELECT" + top + " Name FROM CharacterInfo WHERE AccountName=? ORDER BY Experience DESC" + suffix);
    db->bindParameter<std::string>(1, accountName);
    db->execute();
    while (db->fetch()) {
        auto name = db->getData<std::string>(1);
        if (name && !name->empty()) names.push_back(*name);
    }
    return names;
}

bool Orm::characterBelongsTo(const std::string& accountName, const std::string& characterName) const {
    auto db = m_pool.getConnection(EDatabaseID::UserDB_Primary);
    if (!db) return false;
    db->prepare("SELECT Name FROM CharacterInfo WHERE AccountName=? AND Name=?");
    db->bindParameter<std::string>(1, accountName);
    db->bindParameter<std::string>(2, characterName);
    db->execute();
    return db->fetch();
}

bool Orm::createCharacterRecord(const std::string& accountName, const std::string& characterName) const {
    auto db = m_pool.getConnection(EDatabaseID::UserDB_Primary);
    if (!db || characterBelongsTo(accountName, characterName)) return false;
    db->prepare("INSERT INTO CharacterInfo (Name, AccountName, Experience) VALUES (?, ?, 0)");
    db->bindParameter<std::string>(1, characterName);
    db->bindParameter<std::string>(2, accountName);
    db->execute();
    return true;
}

bool Orm::deleteCharacterRecord(const std::string& accountName, const std::string& characterName) const {
    auto db = m_pool.getConnection(EDatabaseID::UserDB_Primary);
    if (!db || !characterBelongsTo(accountName, characterName)) return false;
    db->prepare("DELETE FROM CharacterInfo WHERE AccountName=? AND Name=?");
    db->bindParameter<std::string>(1, accountName);
    db->bindParameter<std::string>(2, characterName);
    db->execute();
    return true;
}
