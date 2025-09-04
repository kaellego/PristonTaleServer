#pragma once

#include <string>
#include <vector>
#include <optional>
#include "Shared/datatypes.h"

// Forward declarations
class DatabaseManager;
class PlayerRepository;
class LogService;

/**
 * @class CharacterService
 * @brief Gerencia a lógica de negócio para os personagens dos jogadores.
 */
class CharacterService {
public:
    /**
     * @brief Construtor com Injeção de Dependência.
     */
    explicit CharacterService(DatabaseManager& dbManager, PlayerRepository& playerRepo, LogService& logService);

    /**
     * @brief Obtém a lista de personagens para uma determinada conta.
     * @param accountId O ID da conta.
     * @return Um vetor de structs CharacterData.
     */
    std::vector<CharacterData> getCharacterList(int accountId);

private:
    DatabaseManager& m_dbManager;
    PlayerRepository& m_playerRepository;
    LogService& m_logService;
};