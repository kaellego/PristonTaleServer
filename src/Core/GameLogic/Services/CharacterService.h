#pragma once

#include <string>
#include <vector>
#include <optional>
#include "Shared/datatypes.h"

// Forward declarations
class DatabasePool;
class PlayerRepository;
class LogService;

/**
 * @class CharacterService
 * @brief Gerencia a lÃ³gica de negÃ³cio para os personagens dos jogadores.
 */
class CharacterService {
public:
    /**
     * @brief Construtor com InjeÃ§Ã£o de DependÃªncia.
     */
    explicit CharacterService(DatabasePool& dbPool, PlayerRepository& playerRepo, LogService& logService);

    /**
     * @brief ObtÃ©m a lista de personagens para uma determinada conta.
     * @param accountId O ID da conta.
     * @return Um vetor de structs CharacterData.
     */
    std::vector<CharacterData> getCharacterList(const std::string& accountName);
    bool createCharacter(const std::string& accountName, const std::string& characterName, int characterClass);
    bool deleteCharacter(const std::string& accountName, const std::string& characterName);
    std::optional<CharacterData> selectCharacter(const std::string& accountName, const std::string& characterName) const;
    bool ownsCharacter(const std::string& accountName, const std::string& characterName) const;

private:
    DatabasePool& m_dbPool;
    PlayerRepository& m_playerRepository;
    LogService& m_logService;
};
