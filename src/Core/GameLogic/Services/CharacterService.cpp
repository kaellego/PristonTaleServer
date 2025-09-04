#include "GameLogic/Services/CharacterService.h"
#include "Database/DatabaseManager.h"
#include "Database/SQLConnection.h"
#include "GameLogic/PlayerRepository.h"
#include "Logging/LogService.h"

CharacterService::CharacterService(DatabaseManager& dbManager, PlayerRepository& playerRepo, LogService& logService)
    : m_dbManager(dbManager),
    m_playerRepository(playerRepo),
    m_logService(logService)
{}

std::vector<CharacterData> CharacterService::getCharacterList(int accountId) {
    std::vector<CharacterData> characterList;

    auto db = m_dbManager.createConnection(EDatabaseID::UserDB_Primary);
    if (!db) {
        m_logService.error("Nao foi possivel obter conexao com UserDB_Primary em CharacterService.");
        return characterList;
    }

    try {
        // Busca os nomes de até 6 personagens da conta, ordenados por experiência
        db->prepare("SELECT TOP 6 Name FROM CharacterInfo WHERE AccountID=? ORDER BY Experience DESC");
        db->bindParameter<int>(1, accountId);

        db->execute();

        while (db->fetch()) {
            auto charNameOpt = db->getData<std::string>(1);
            if (charNameOpt.has_value()) {
                // Para cada nome encontrado, carrega os dados completos do arquivo .chr
                auto charDataOpt = m_playerRepository.loadCharacter(*charNameOpt);
                if (charDataOpt.has_value()) {
                    characterList.push_back(*charDataOpt);
                }
                else {
                    m_logService.warn("Personagem '{}' encontrado no DB, mas o arquivo .chr nao foi encontrado ou falhou ao carregar.", *charNameOpt);
                }
            }
        }
    }
    catch (const std::exception& e) {
        m_logService.error("Erro de SQL em getCharacterList para a conta ID {}: {}", accountId, e.what());
    }

    m_logService.info("Encontrado(s) {} personagem(ns) para a conta ID {}.", characterList.size(), accountId);
    return characterList;
}