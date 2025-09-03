#pragma once

#include <string>
#include <cstdint>

/**
 * @namespace Log
 * @brief Agrupa constantes e funções relacionadas a eventos de log.
 */
namespace Log {

    /**
     * @enum class LoginResult
     * @brief Códigos de resultado retornados ao cliente durante o login.
     * São valores negativos para indicar erro.
     */
    enum class LoginResult : int {
        Maintenance = -8,
        IncorrectPassword = -2,
        IncorrectAccount = -1,
        BlockedAccount = -3,
        AlreadyLogged = -4,
        Banned = -6,
        TempBanned = -18,
        NotActivated = -16,
        UnauthorizedMaintenance = 0,
        UnknownError = 0,
        Success = 1 // Embora não seja um ID de mensagem, é um resultado válido.
    };

    /**
     * @enum class AccountEvent
     * @brief Identificadores para eventos de conta registrados no banco de dados.
     */
    enum class AccountEvent : int {
        LoginSuccess = 501,
        IncorrectAccount = 502,
        IncorrectPassword = 503,
        BlockedAccount = 504,
        NotActivatedAccount = 505,
        CharacterSelectSend = 506,
        CharacterCreated = 507,
        ReceivedPostBox = 508,
        CharacterDeleted = 509,
        UnblockedAccount = 510,
        CharacterSave = 511,
        TempBanAccount = 513,
        CharacterQuestGot = 801,     // 500 + 301
        CharacterQuestCancel = 802,  // 500 + 302
        CharacterQuestFinished = 803,// 500 + 303
        CharacterQuestEXP = 804,     // 500 + 304
        CharacterExpLose = 805,      // 500 + 305
        CharacterLoad = 806          // 500 + 306
    };

    // --- Funções de Formatação de Log (Substitutas das Macros) ---

    // Formata logs de login
    std::string formatLoginSuccess(const std::string& accountName, const std::string& password, const std::string& macAddr, const std::string& pcName, int serialHD, const std::string& videoName);
    std::string formatIncorrectAccount(const std::string& accountName, const std::string& password, const std::string& macAddr, const std::string& pcName, int serialHD, const std::string& videoName);
    std::string formatIncorrectPassword(const std::string& accountName, const std::string& password);
    std::string formatAccountNotActivated(const std::string& accountName, const std::string& password);
    std::string formatAccountBanned(const std::string& accountName, const std::string& password);
    std::string formatAccountUnbanned(const std::string& accountName);

    // Formata logs de personagem
    std::string formatCharacterSelect(const std::string& accountName);
    std::string formatCharacterCreated(const std::string& accountName, const std::string& charName);
    std::string formatExpReceived(const std::string& charName, int64_t exp, int level, int mapId, const std::string& partyInfo);
    std::string formatExpMaxReceived(const std::string& charName, int64_t exp, int level, int mapId, const std::string& partyInfo);
    std::string formatExpLost(const std::string& charName, int64_t exp, int level, int mapId, const std::string& partyInfo);

    // Formata logs de PostBox
    std::string formatPostBoxItemReceived(const std::string& charName, const std::string& itemName, int amount);

} // namespace Log