#include "GameLogic/Services/CharacterService.h"
#include "Database/DatabasePool.h"
#include "GameLogic/PlayerRepository.h"
#include "Logging/LogService.h"
#include "Database/Orm.h"

#include <cctype>
#include <algorithm>
#include <array>
#include <cmath>
#include "Utils/SafeString.h"

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

void applyStarterStats(CharacterData& character, int characterClass) {
    // These are the level-1 values used by the original client's character
    // creation screen. Do not apply them to a character that already has real
    // progression; this only repairs empty/default state.
    static constexpr std::array<std::array<int32_t, 5>, 12> stats{{
        {{1, 1, 1, 1, 1}},
        {{24, 8, 25, 18, 24}}, // Fighter
        {{28, 6, 21, 17, 27}}, // Mechanician
        {{17, 11, 21, 27, 23}}, // Archer
        {{26, 9, 20, 19, 25}}, // Pikeman
        {{26, 13, 17, 19, 24}}, // Knight
        {{23, 15, 19, 19, 23}}, // Atalanta
        {{15, 28, 21, 15, 20}}, // Priestess
        {{16, 29, 19, 14, 21}}, // Magician
        {{17, 11, 21, 27, 23}}, // Assassin: same base family as Archer
        {{16, 29, 19, 14, 21}}, // Shaman: same base family as Magician
        {{24, 8, 25, 18, 24}}   // Martial Artist: same base family as Fighter
    }};
    const auto index = std::clamp(characterClass, 1, 11);
    const auto& values = stats[static_cast<size_t>(index)];
    character.iStrength = values[0];
    character.iSpirit = values[1];
    character.iTalent = values[2];
    character.iAgility = values[3];
    character.iHealth = values[4];
}

CharacterData defaultCharacter(const std::string& name) {
    CharacterData character{};
    copyFixed(character.szName, name);
    character.iLevel = 1;
    character.iClass = 1;
    character.iHP = character.iMaxHP = 100;
    character.iMP = character.iMaxMP = 100;
    character.iSTM = character.iMaxSTM = 100;
    applyStarterStats(character, character.iClass);
    character.iAttackMin = character.iAttackMax = 10;
    return character;
}

void normalizeForRuntime(CharacterData& character, const std::string& canonicalName) {
    // The SQL row is the authoritative identity. A stale blob must not change
    // the name shown by the legacy client or the file selected by later saves.
    character.szName[0] = '\0';
    copyFixed(character.szName, canonicalName);
    if (character.iClass < 1 || character.iClass > 11) character.iClass = 1;
    if (character.iStrength <= 1 && character.iSpirit <= 1 && character.iTalent <= 1 &&
        character.iAgility <= 1 && character.iHealth <= 1)
        applyStarterStats(character, character.iClass);
    character.iLevel = std::clamp(character.iLevel, 1, 150);
    if (!std::isfinite(character.fPosX)) character.fPosX = 0.0f;
    if (!std::isfinite(character.fPosY)) character.fPosY = 0.0f;
    if (!std::isfinite(character.fPosZ)) character.fPosZ = 0.0f;
    if (character.iMapId < 0 || character.iMapId > 255) character.iMapId = 0;

    character.iMaxHP = std::max(100, character.iMaxHP);
    character.iHP = std::clamp(character.iHP > 0 ? character.iHP : character.iMaxHP, 1, character.iMaxHP);
    character.iMaxMP = std::max(100, character.iMaxMP);
    character.iMP = std::clamp(character.iMP > 0 ? character.iMP : character.iMaxMP, 1, character.iMaxMP);
    character.iMaxSTM = std::max(100, character.iMaxSTM);
    character.iSTM = std::clamp(character.iSTM > 0 ? character.iSTM : character.iMaxSTM, 1, character.iMaxSTM);
}
}

std::optional<CharacterData> CharacterService::loadOrRecoverCharacter(const std::string& characterName) const {
    if (auto character = m_playerRepository.loadCharacter(characterName)) {
        normalizeForRuntime(*character, characterName);
        return character;
    }
    if (m_playerRepository.characterFileExists(characterName)) {
        m_logService.warn("Arquivo .chr invalido para '{}'; recuperacao automatica bloqueada para preservar os dados.", characterName);
        return std::nullopt;
    }

    auto recovered = defaultCharacter(characterName);
    if (!m_playerRepository.saveCharacter(recovered)) {
        m_logService.error("Nao foi possivel criar o arquivo .chr recuperado para '{}'.", characterName);
        return std::nullopt;
    }
    m_logService.warn("Personagem '{}' recuperado do banco sem .chr; classe inicial padrao 1 aplicada.", characterName);
    return recovered;
}

std::vector<CharacterData> CharacterService::getCharacterList(const std::string& accountName) {
    std::vector<CharacterData> characterList;
    try {
        Orm orm(m_dbPool, m_logService, m_dbPool.dialect());
        for (const auto& charName : orm.findCharacterNames(accountName)) {
            auto charData = loadOrRecoverCharacter(charName);
            if (charData) characterList.push_back(*charData);
            else m_logService.warn("Personagem '{}' nao pode ser carregado com seguranca.", charName);
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
    CharacterData character = defaultCharacter(characterName);
    character.iClass = characterClass;
    applyStarterStats(character, characterClass);
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
    return loadOrRecoverCharacter(characterName);
}

bool CharacterService::ownsCharacter(const std::string& accountName, const std::string& characterName) const {
    Orm orm(m_dbPool, m_logService, m_dbPool.dialect());
    return !accountName.empty() && validCharacterName(characterName) && orm.characterBelongsTo(accountName, characterName);
}
