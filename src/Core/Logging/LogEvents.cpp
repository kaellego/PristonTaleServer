#include "Logging/LogEvents.h"
#include <sstream>

namespace Log {

    std::string formatLoginSuccess(const std::string& accountName, const std::string& password, const std::string& macAddr, const std::string& pcName, int serialHD, const std::string& videoName) {
        std::stringstream ss;
        ss << "[" << accountName << "] (Login Success) -> Password[" << password << "] MacAddr[" << macAddr
            << "] PCName[" << pcName << "] SerialHD[" << serialHD << "] VideoName[" << videoName << "]";
        return ss.str();
    }

    std::string formatIncorrectAccount(const std::string& accountName, const std::string& password, const std::string& macAddr, const std::string& pcName, int serialHD, const std::string& videoName) {
        std::stringstream ss;
        ss << "Unknown_Account (Incorrect Account) -> AccountName[" << accountName << "] Password[" << password
            << "] MacAddr[" << macAddr << "] PCName[" << pcName << "] SerialHD[" << serialHD << "] VideoName[" << videoName << "]";
        return ss.str();
    }

    std::string formatIncorrectPassword(const std::string& accountName, const std::string& password) {
        std::stringstream ss;
        ss << accountName << " (Incorrect Password) -> Password \"" << password << "\"";
        return ss.str();
    }

    std::string formatAccountNotActivated(const std::string& accountName, const std::string& password) {
        std::stringstream ss;
        ss << accountName << " (Account not activated) -> Password \"" << password << "\"";
        return ss.str();
    }

    std::string formatAccountBanned(const std::string& accountName, const std::string& password) {
        std::stringstream ss;
        ss << accountName << " (Account is banned) -> Password \"" << password << "\"";
        return ss.str();
    }

    std::string formatAccountUnbanned(const std::string& accountName) {
        std::stringstream ss;
        ss << accountName << " (Account is unbanned)";
        return ss.str();
    }

    std::string formatCharacterSelect(const std::string& accountName) {
        std::stringstream ss;
        ss << accountName << " (Character Select Time)";
        return ss.str();
    }

    std::string formatCharacterCreated(const std::string& accountName, const std::string& charName) {
        std::stringstream ss;
        ss << accountName << " (New Character Created) -> Name \"" << charName << "\"";
        return ss.str();
    }

    std::string formatExpReceived(const std::string& charName, int64_t exp, int level, int mapId, const std::string& partyInfo) {
        std::stringstream ss;
        ss << "(" << charName << ") (Received Experience) [" << exp << "] Level[" << level
            << "] Map[" << mapId << "] Party[" << partyInfo << "]";
        return ss.str();
    }

    std::string formatExpMaxReceived(const std::string& charName, int64_t exp, int level, int mapId, const std::string& partyInfo) {
        std::stringstream ss;
        ss << "(" << charName << ") (Received Max Experience) [" << exp << "] Level[" << level
            << "] Map[" << mapId << "] Party[" << partyInfo << "]";
        return ss.str();
    }

    std::string formatExpLost(const std::string& charName, int64_t exp, int level, int mapId, const std::string& partyInfo) {
        std::stringstream ss;
        ss << "(" << charName << ") (Lose Experience in Penality) [" << exp << "] Level[" << level
            << "] Map[" << mapId << "] Party[" << partyInfo << "]";
        return ss.str();
    }

    std::string formatPostBoxItemReceived(const std::string& charName, const std::string& itemName, int amount) {
        std::stringstream ss;
        ss << "(" << charName << ") (Received Item) [" << itemName << "] [" << amount << "]";
        return ss.str();
    }

} // namespace Log