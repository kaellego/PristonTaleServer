#include "GameLogic/Services/CharacterService.h"
#include "Database/DatabasePool.h"
#include "Database/SQLConnection.h"
#include "GameLogic/PlayerRepository.h"
#include "Logging/LogService.h"

CharacterService::CharacterService(DatabasePool& dbPool, PlayerRepository& playerRepo, LogService& logService)
    : m_dbPool(dbPool),
    m_playerRepository(playerRepo),
    m_logService(logService)
{}

std::vector<CharacterData> CharacterService::getCharacterList(const std::string& accountName) {
    std::vector<CharacterData> characterList;

    auto db = m_dbPool.getConnection(EDatabaseID::UserDB_Primary);
    if (!db) {
        // Esta parte só será executada se, por algum motivo, o pool não puder
        // fornecer uma conexão válida (o que é improvável no design atual,
        // mas é uma excelente verificação de segurança).
        m_logService.error("Nao foi possivel obter conexao com UserDB_Primary em CharacterService.");
        return characterList;
    }

    try {
        // --- A CONSULTA SQL CORRIGIDA ---
        // Agora busca por AccountName em vez de AccountID
        db->prepare("SELECT TOP 6 Name FROM CharacterInfo WHERE AccountName=? ORDER BY Experience DESC");
        db->bindParameter<std::string>(1, accountName);

        db->execute();

        while (db->fetch()) {
            auto charNameOpt = db->getData<std::string>(1);
            if (charNameOpt.has_value() && !charNameOpt->empty()) {
                auto charDataOpt = m_playerRepository.loadCharacter(*charNameOpt);
                if (charDataOpt.has_value()) {
                    characterList.push_back(*charDataOpt);
                }
                else {
                    m_logService.warn("Personagem '{}' encontrado no DB, mas o arquivo .chr nao foi encontrado.", *charNameOpt);
                }
            }
        }
    }
    catch (const std::exception& e) {
        m_logService.error("Erro de SQL em getCharacterList para a conta {}: {}", accountName, e.what());
    }

    m_logService.info("Encontrado(s) {} personagem(ns) para a conta {}.", characterList.size(), accountName);
    return characterList;
}