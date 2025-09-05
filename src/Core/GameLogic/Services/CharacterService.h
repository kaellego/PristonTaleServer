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
 * @brief Gerencia a l�gica de neg�cio para os personagens dos jogadores.
 */
class CharacterService {
public:
    /**
     * @brief Construtor com Inje��o de Depend�ncia.
     */
    explicit CharacterService(DatabasePool& dbPool, PlayerRepository& playerRepo, LogService& logService);

    /**
     * @brief Obt�m a lista de personagens para uma determinada conta.
     * @param accountId O ID da conta.
     * @return Um vetor de structs CharacterData.
     */
    std::vector<CharacterData> getCharacterList(const std::string& accountName);

private:
    DatabasePool& m_dbPool;
    PlayerRepository& m_playerRepository;
    LogService& m_logService;
};