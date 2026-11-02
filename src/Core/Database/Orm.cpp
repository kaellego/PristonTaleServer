#include "Database/Orm.h"
#include "Utils/SafeString.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "Logging/LogService.h"

Orm::Orm(DatabasePool& pool, LogService& log, DatabaseDialect dialect) : m_pool(pool), m_log(log), m_dialect(dialect) {}

std::optional<SQLUser> Orm::findUser(const std::string& accountName) const {
    auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary);
    db->prepare("SELECT TOP 1 ID, AccountName, Password, Flag, BanStatus, IsMuted FROM UserInfo WHERE AccountName=?");
    db->bindParameter(1, accountName); db->execute(); if (!db->fetch()) return std::nullopt;
    SQLUser user{}; user.iID = db->getData<int>(1).value_or(0);
    auto name = db->getData<std::string>(2).value_or(""); auto password = db->getData<std::string>(3).value_or("");
    copyFixed(user.szAccountName, name);
    copyFixed(user.szPassword, password);
    user.iFlag = db->getData<int>(4).value_or(0); user.iBanStatus = static_cast<EBanStatus>(db->getData<int>(5).value_or(0)); user.bIsMuted = db->getData<int>(6).value_or(0) != 0;
    return user;
}

std::vector<std::string> Orm::findCharacterNames(const std::string& accountName, int limit) const {
    std::vector<std::string> names; auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary);
    db->prepare("SELECT TOP (?) Name FROM CharacterInfo WHERE AccountName=? ORDER BY Experience DESC"); db->bindParameter(1, limit); db->bindParameter(2, accountName); db->execute();
    while (db->fetch()) if (auto name = db->getData<std::string>(1); name && !name->empty()) names.push_back(*name); return names;
}

bool Orm::characterBelongsTo(const std::string& accountName, const std::string& characterName) const {
    auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->prepare("SELECT TOP 1 Name FROM CharacterInfo WHERE AccountName=? AND Name=?"); db->bindParameter(1, accountName); db->bindParameter(2, characterName); db->execute(); return db->fetch();
}

bool Orm::createCharacterRecord(const std::string& accountName, const std::string& characterName) const {
    if (characterBelongsTo(accountName, characterName)) return false; auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->prepare("INSERT INTO CharacterInfo (Name, AccountName, Experience) VALUES (?, ?, 0)"); db->bindParameter(1, characterName); db->bindParameter(2, accountName); db->execute(); return true;
}

bool Orm::deleteCharacterRecord(const std::string& accountName, const std::string& characterName) const {
    if (!characterBelongsTo(accountName, characterName)) return false; auto db = m_pool.createConnection(EDatabaseID::UserDB_Primary); db->prepare("DELETE FROM CharacterInfo WHERE AccountName=? AND Name=?"); db->bindParameter(1, accountName); db->bindParameter(2, characterName); db->execute(); return true;
}
