#include "GameLogic/Services/CharacterService.h"
#include "Database/DatabasePool.h"
#include "GameLogic/PlayerRepository.h"
#include "Logging/LogService.h"
#include "Database/Orm.h"

#include <cctype>
#include <algorithm>

CharacterService::CharacterService(DatabasePool& dbPool, PlayerRepository& playerRepo, LogService& logService)
    : m_dbPool(dbPool),
    m_playerRepository(playerRepo),
    m_logService(logService)
{}

namespace {
bool validCharacterName(const std::string& name) {
    if (name.empty() || name.size() >= 32) return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}
}

std::vector<CharacterData> CharacterService::getCharacterList(const std::string& accountName) {
    std::vector<CharacterData> characterList;
    try {
        Orm orm(m_dbPool, m_logService, m_dbPool.dialect());
        for (const auto& charName : orm.findCharacterNames(accountName)) {
            auto charData = m_playerRepository.loadCharacter(charName);
            if (charData) characterList.push_back(*charData);
            else m_logService.warn("Personagem '{}' encontrado no DB, mas o arquivo .chr nao foi encontrado.", charName);
        }
    } catch (const std::exception& e) {
        m_logService.error("Erro de ORM em getCharacterList para a conta {}: {}", accountName, e.what());
    }
    return characterList;
}
bool CharacterService::createCharacter(const std::string& accountName, const std::string& characterName, int characterClass) {
    if (accountName.empty() || !validCharacterName(characterName) || characterClass < 1 || characterClass > 11) {
        m_logService.warn("Criacao rejeitada: dados invalidos para '{}' (classe {}).", characterName, characterClass);
        return false;
    }
    Orm orm(m_dbPool, m_logService, m_dbPool.dialect());
    if (orm.findCharacterNames(accountName, 7).size() >= 6) {
        m_logService.warn("Criacao rejeitada para {}: limite de 6 personagens atingido.", accountName);
        return false;
    }
    if (orm.characterBelongsTo(accountName, characterName)) {
        m_logService.warn("Criacao rejeitada para {}: personagem '{}' ja existe.", accountName, characterName);
        return false;
    }
    CharacterData character{};
    strncpy_s(character.szName, sizeof(character.szName), characterName.c_str(), _TRUNCATE);
    character.iLevel = 1;
    character.liEXP = 0;
    character.iClass = characterClass;
    character.iMapId = 0;
    character.iHP = character.iMaxHP = 100;
    character.iMP = character.iMaxMP = 100;
    character.iStrength = character.iSpirit = character.iTalent = character.iAgility = character.iHealth = 1;
    character.iAttackMin = character.iAttackMax = 10; character.iDefense = 0;
    if (!m_playerRepository.saveCharacter(character)) {
        m_logService.error("Criacao rejeitada para {}: falha ao salvar o arquivo de '{}'.", accountName, characterName);
        return false;
    }
    if (!orm.createCharacterRecord(accountName, characterName)) {
        m_playerRepository.deleteCharacter(characterName);
        m_logService.error("Criacao rejeitada para {}: falha ao criar o registro de '{}' no banco.", accountName, characterName);
        return false;
    }
    return true;
}

bool CharacterService::deleteCharacter(const std::string& accountName, const std::string& characterName) {
    if (accountName.empty() || !validCharacterName(characterName)) return false;
    Orm orm(m_dbPool, m_logService, m_dbPool.dialect());
    if (!orm.characterBelongsTo(accountName, characterName)) return false;
    if (!orm.deleteCharacterRecord(accountName, characterName)) return false;
    return m_playerRepository.deleteCharacter(characterName);
}

std::optional<CharacterData> CharacterService::selectCharacter(const std::string& accountName, const std::string& characterName) const {
    if (accountName.empty() || !validCharacterName(characterName)) return std::nullopt;
    Orm orm(m_dbPool, m_logService, m_dbPool.dialect());
    if (!orm.characterBelongsTo(accountName, characterName)) return std::nullopt;
    return m_playerRepository.loadCharacter(characterName);
}

bool CharacterService::ownsCharacter(const std::string& accountName, const std::string& characterName) const {
    Orm orm(m_dbPool, m_logService, m_dbPool.dialect());
    return !accountName.empty() && validCharacterName(characterName) && orm.characterBelongsTo(accountName, characterName);
}
