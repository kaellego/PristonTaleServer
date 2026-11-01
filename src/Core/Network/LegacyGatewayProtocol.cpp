#include "Network/LegacyGatewayProtocol.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"

#include <algorithm>
#include <cstring>

namespace LegacyGateway {

namespace {

#pragma pack(push, 1)
struct LegacyCharacterInfo {
    char name[32];
    char modelName[64];
    char modelName2[60];
    uint32_t modelNameCode2;
    uint32_t objectSerial;
    int32_t classClan, state, sizeLevel;
    uint32_t charSoundCode;
    uint32_t jobCode;
    int32_t level, strength, spirit, talent, dexterity, health;
    int32_t accuracy, attackRating, attackDamage[2], attackSpeed, shootingRange, criticalHit;
    int32_t defense, chanceBlock, absorption, moveSpeed, sight;
    int16_t weight[2], resistance[8], attackResistance[8], life[2], mana[2], stamina[2];
    float lifeRegen, manaRegen, staminaRegen;
    int32_t experience, nextExperience, money;
    uint32_t monsterInfo;
    uint32_t brood;
    int32_t statePoint;
    uint8_t updateInfo[4];
    int16_t arrowPosition[2];
    int32_t potionSpace, lifeFunction, manaFunction, staminaFunction;
    int16_t damageFunction[2];
    uint32_t reformCode, changeJob, jobBitMask;
    uint16_t playerKilling[2], playClass[2];
    int32_t experienceHigh;
    uint32_t eventTime;
    int16_t eventParam[2], presentItem[2], gravityScrollCheck[2];
    uint32_t temp[11], loginServerIp, loginServerSafeKey;
    uint16_t version[2];
};

struct LegacyRecordSkill {
    uint8_t skillPoint[16];
    uint16_t skillMastery[16];
    uint8_t shortKey[16];
    uint16_t selectedSkill[2];
};

struct LegacyQuestInfo {
    uint16_t questCode[2];
    uint32_t data[7];
};

struct LegacyLastQuestInfo {
    uint16_t lastQuest[32];
    int32_t lastQuestCount;
};

struct LegacyGameSaveInfo {
    uint32_t head;
    int32_t playStage, cameraMode, posX, posZ, lastMoney;
    uint32_t charInfoChecksum;
    LegacyRecordSkill skillInfo;
    int32_t saveTime, pcrNo;
    int16_t eventPlay[2], petInfo[2], potionUpdate[2], potionCount[3][4];
    uint16_t forceOrbUsing[2];
    uint32_t levelQuestLog;
    int32_t shortKeyNormalAttack, blessCastleTax;
    uint32_t blessCastleMaster, elementaryQuestLog;
    uint32_t primeItemTimes[12];
    int32_t totalSubExp, totalMoney, totalExp;
    char masterId[32];
    LegacyQuestInfo questInfo;
    LegacyLastQuestInfo lastQuestInfo;
    uint32_t serverId;
    uint16_t lifeBoosterUsing[2], manaBoosterUsing[2], staminaBoosterUsing[2], skillDelayUsing[2];
    uint32_t temp2[34];
};

struct LegacyRecordPrefix {
    int32_t size;
    int32_t code;
    char header[8];
    LegacyCharacterInfo character;
    LegacyGameSaveInfo save;
    uint32_t thrownItems[64][3];
    int32_t throwItemCount, itemCount, itemSubStart, dataSize;
};
#pragma pack(pop)

static_assert(sizeof(LegacyCharacterInfo) == 464);
static_assert(sizeof(LegacyGameSaveInfo) == 512);

const char* modelBodyForClass(int characterClass) {
    static constexpr const char* models[] = {
        "char\\tmABCD\\b001.ini", "char\\tmABCD\\a001.ini", "char\\tmABCD\\d001.ini", "char\\tmABCD\\c001.ini",
        "char\\tmABCD\\mb001.ini", "char\\tmABCD\\ma001.ini", "char\\tmABCD\\md001.ini", "char\\tmABCD\\mc001.ini"};
    return characterClass >= 1 && characterClass <= 8 ? models[characterClass - 1] : models[0];
}

const char* modelFaceForClass(int characterClass) {
    static constexpr const char* models[] = {
        "char\\tmABCD\\tmh-b01.inf", "char\\tmABCD\\tmh-a01.inf", "char\\tmABCD\\tfh-D01.inf", "char\\tmABCD\\tmh-c01.inf",
        "char\\tmABCD\\Mfh-B01.inf", "char\\tmABCD\\Mmh-A01.inf", "char\\tmABCD\\Mmh-D01.inf", "char\\tmABCD\\Mfh-C01.inf"};
    return characterClass >= 1 && characterClass <= 8 ? models[characterClass - 1] : models[0];
}

std::vector<uint8_t> characterRecordPayload(const CharacterData& character) {
    LegacyRecordPrefix record{};
    record.code = static_cast<int32_t>(Opcode::RecordData);
    record.size = static_cast<int32_t>(sizeof(record));
    std::memcpy(record.character.name, character.szName, sizeof(record.character.name));
    strncpy_s(record.character.modelName, modelBodyForClass(character.iClass), _TRUNCATE);
    strncpy_s(record.character.modelName2, modelFaceForClass(character.iClass), _TRUNCATE);
    record.character.objectSerial = 0x10000000u + static_cast<uint32_t>(std::hash<std::string>{}(character.szName));
    record.character.jobCode = static_cast<uint32_t>(character.iClass);
    record.character.level = std::max(1, character.iLevel);
    record.character.strength = character.iStrength;
    record.character.spirit = character.iSpirit;
    record.character.talent = character.iTalent;
    record.character.dexterity = character.iAgility;
    record.character.health = character.iHealth;
    record.character.attackDamage[0] = character.iAttackMin;
    record.character.attackDamage[1] = character.iAttackMax;
    record.character.defense = character.iDefense;
    record.character.life[0] = static_cast<int16_t>(std::clamp(character.iHP, 0, 32767));
    record.character.life[1] = static_cast<int16_t>(std::clamp(character.iMaxHP, 1, 32767));
    record.character.mana[0] = static_cast<int16_t>(std::clamp(character.iMP, 0, 32767));
    record.character.mana[1] = static_cast<int16_t>(std::clamp(character.iMaxMP, 1, 32767));
    record.character.money = character.iGold;
    record.character.version[0] = 1;
    record.save.head = 150;
    record.save.playStage = character.iMapId;
    record.save.posX = static_cast<int32_t>(character.fPosX);
    record.save.posZ = static_cast<int32_t>(character.fPosZ);
    record.save.totalMoney = character.iGold;

    std::vector<uint8_t> frame(sizeof(record) + 32, 0);
    const int32_t frameSize = static_cast<int32_t>(frame.size());
    const int32_t total = 1;
    const int32_t dataSize = static_cast<int32_t>(sizeof(record));
    std::memcpy(frame.data(), &frameSize, sizeof(frameSize));
    std::memcpy(frame.data() + 4, &record.code, sizeof(record.code));
    std::memcpy(frame.data() + 12, &total, sizeof(total));
    std::memcpy(frame.data() + 16, &dataSize, sizeof(dataSize));
    std::memcpy(frame.data() + 20, &record, sizeof(record));
    return frame;
}

} // namespace

bool isSafeString(const char* value, size_t capacity) {
    return value != nullptr && std::memchr(value, '\0', capacity) != nullptr;
}

std::optional<Packet> toModernPacket(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(Command) || payload.size() > Constants::MAX_PKTSIZ) return std::nullopt;

    Command command{};
    std::memcpy(&command, payload.data(), sizeof(command));
    if (command.size < static_cast<int32_t>(sizeof(Command)) ||
        command.size > static_cast<int32_t>(payload.size())) return std::nullopt;

    // Only explicitly supported login-stage commands are translated. Unknown
    // legacy opcodes are rejected instead of being guessed.
    uint32_t modernOpcode = 0;
    switch (static_cast<Opcode>(command.code)) {
    case Opcode::Connected:
        modernOpcode = static_cast<uint32_t>(Opcodes::KeepAlive);
        break;
    case Opcode::RecordData: {
        // TRANS_RECORD_DATAS: five integers, serialized record and 12 legacy padding bytes.
        if (command.size != static_cast<int32_t>(payload.size()) || command.size < 32 ||
            command.lParam < 0 || command.wParam <= 0 || command.wParam > 64 ||
            command.lParam >= command.wParam || command.sParam <= 0 ||
            command.sParam > command.size - 32)
            return std::nullopt;
        modernOpcode = static_cast<uint32_t>(Opcodes::KeepAlive);
        break;
    }
    case Opcode::SetBlacklist:
        if (command.size != sizeof(Command) || payload.size() != sizeof(Command))
            return std::nullopt;
        // The legacy client reports local protection status through this command.
        // Do not trust or execute its values; only acknowledge its fixed layout.
        modernOpcode = static_cast<uint32_t>(Opcodes::KeepAlive);
        break;
    case Opcode::PlayerInfo:
        // smTRNAS_PLAYERINFO uses the 32-bit legacy smCHAR_INFO layout: 504 bytes.
        if (command.size != 504 || payload.size() != 504)
            return std::nullopt;
        modernOpcode = static_cast<uint32_t>(Opcodes::KeepAlive);
        break;
    case Opcode::ProcessInfo:
        // TRANS_CHATMESSAGE carries two DWORDs followed by a bounded process path.
        if (command.size != static_cast<int32_t>(payload.size()) || command.size < 17 ||
            command.size > 512 || !isSafeString(reinterpret_cast<const char*>(payload.data() + 16), payload.size() - 16))
            return std::nullopt;
        modernOpcode = static_cast<uint32_t>(Opcodes::KeepAlive);
        break;
    case Opcode::GetUserInfo:
        return loginToModernPacket(payload);
    case Opcode::CheckIP:
        if (command.size != 52 || command.wParam < 0 || command.wParam > 3 ||
            !isSafeString(reinterpret_cast<const char*>(payload.data() + 20), 32))
            return std::nullopt;
        return Packet(static_cast<uint32_t>(Opcodes::KeepAlive));
    case Opcode::FunctionResult:
        // The legacy client sends this fixed command for its client-function
        // check. Validate the frame and continue; the modern server has no
        // executable-memory probe to run on the client.
        if (command.size != sizeof(Command) || payload.size() < sizeof(Command))
            return std::nullopt;
        return Packet(static_cast<uint32_t>(Opcodes::KeepAlive));
    default:
        // The legacy client uses GETRECORDDATA (opcode 5) on the login/data
        // socket when it requests the selected character after choosing a
        // server. Translate only its fixed, bounded command layout.
        if ((command.code == 5 && command.size == 52 && payload.size() >= 52) ||
            ((command.code == 10 || command.code == 11) && command.size == 76 && payload.size() >= 76)) {
            PacketCharacterCommand modern{};
            modern.header.opcode = static_cast<uint32_t>(
                command.code == 5 ? Opcodes::SelectCharacter :
                command.code == 10 ? Opcodes::DeleteCharacter : Opcodes::CreateCharacter);
            std::memcpy(&modern.LParam, payload.data() + 8, sizeof(modern.LParam));
            std::memcpy(&modern.WParam, payload.data() + 12, sizeof(modern.WParam));
            std::memcpy(&modern.SParam, payload.data() + 16, sizeof(modern.SParam));
            if (command.code == 11 && modern.LParam == 0)
                modern.LParam = 1; // Legacy creation packets omit the class; use the standard first class.
            if (command.code != 5)
                std::memcpy(modern.szAccount, payload.data() + 20, 21);
            const auto* name = reinterpret_cast<const char*>(payload.data() + (command.code == 5 ? 20 : 41));
            std::memcpy(modern.szName, name, sizeof(modern.szName));
            const auto* body = reinterpret_cast<const uint8_t*>(&modern) + sizeof(PacketHeader);
            return Packet(modern.header.opcode,
                std::vector<uint8_t>(body, body + sizeof(modern) - sizeof(PacketHeader)));
        }
        return std::nullopt;
    }

    std::vector<uint8_t> body(payload.begin() + sizeof(Command), payload.begin() + command.size);
    return Packet(modernOpcode, body);
}

std::optional<Packet> loginToModernPacket(const std::vector<uint8_t>& payload) {
    if (payload.size() != sizeof(LoginPacket)) return std::nullopt;

    LoginPacket legacy{};
    std::memcpy(&legacy, payload.data(), sizeof(legacy));
    if (legacy.size != static_cast<int32_t>(sizeof(LoginPacket)) ||
        legacy.code != static_cast<int32_t>(Opcode::GetUserInfo) ||
        !isSafeString(legacy.account, sizeof(legacy.account)) ||
        !isSafeString(legacy.password, sizeof(legacy.password)) ||
        !isSafeString(legacy.hash, sizeof(legacy.hash)) ||
        !isSafeString(legacy.mac, sizeof(legacy.mac))) {
        return std::nullopt;
    }

    PacketLoginUser modern{};
    modern.header.opcode = static_cast<uint32_t>(Opcodes::LoginUser);
    std::memcpy(modern.szUserID, legacy.account, sizeof(legacy.account));
    std::memcpy(modern.szPassword, legacy.password, sizeof(legacy.password));
    std::memcpy(modern.szMacAddr, legacy.mac, sizeof(legacy.mac));
    std::memcpy(modern.szHardwareID, legacy.hash, sizeof(legacy.hash));
    modern.uWidthScreen = 800;
    modern.uHeightScreen = 600;
    modern.iVersion = 1610;

    const auto* body = reinterpret_cast<const uint8_t*>(&modern) + sizeof(PacketHeader);
    return Packet(modern.header.opcode,
        std::vector<uint8_t>(body, body + sizeof(modern) - sizeof(PacketHeader)));
}

std::optional<std::vector<uint8_t>> toLegacyPayload(const Packet& packet, uint32_t packetKey, int32_t requestCode) {
    std::vector<uint8_t> payload;
    auto append = [&payload](const void* data, size_t size) {
        const auto* p = static_cast<const uint8_t*>(data);
        payload.insert(payload.end(), p, p + size);
    };

    if (packet.header.opcode == static_cast<uint32_t>(Opcodes::KeepAlive)) {
        Command command{sizeof(Command), static_cast<int32_t>(Opcode::Connected), 0, 0, 0, 0};
        append(&command, sizeof(command));
    } else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::AccountLoginCode)) {
        if (packet.body.size() < sizeof(uint32_t) + sizeof(int32_t) * 2) return std::nullopt;
        int32_t result = 0;
        std::memcpy(&result, packet.body.data(), sizeof(result));
        if (result == 0) return std::vector<uint8_t>{}; // Success is followed by server/character lists.
        Command command{sizeof(Command), static_cast<int32_t>(Opcode::FailConnect), 0, result, 0, 1};
        append(&command, sizeof(command));
    } else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::ServerList)) {
        constexpr size_t serverListPrefix = offsetof(PacketServerList, sServers) - sizeof(PacketHeader);
        if (packet.body.size() < serverListPrefix) return std::nullopt;
        PacketServerList modern{};
        const size_t copySize = std::min(packet.body.size(), sizeof(modern) - sizeof(PacketHeader));
        std::memcpy(reinterpret_cast<uint8_t*>(&modern) + sizeof(PacketHeader), packet.body.data(), copySize);
        struct LegacyServerInfo { char name[32], ip1[20], ip2[20], ip3[20]; uint32_t port1, port2, port3, state; };
        struct LegacyServerList { int32_t size, code; char name[16]; uint32_t time; int32_t clanTicket, temp, clanCount, count; LegacyServerInfo servers[8]; } legacy{};
        legacy.size = sizeof(legacy);
        legacy.code = static_cast<int32_t>(Opcode::ServerInfo);
        std::memcpy(legacy.name, modern.szServerName, sizeof(legacy.name));
        legacy.clanCount = 0;
        legacy.count = std::min(modern.iGameServers, 8);
        for (int i = 0; i < legacy.count; ++i) {
            std::memcpy(legacy.servers[i].name, modern.sServers[i].szName, sizeof(legacy.servers[i].name));
            std::memcpy(legacy.servers[i].ip1, modern.sServers[i].szaIP[0], sizeof(legacy.servers[i].ip1));
            std::memcpy(legacy.servers[i].ip2, modern.sServers[i].szaIP[1], sizeof(legacy.servers[i].ip2));
            std::memcpy(legacy.servers[i].ip3, modern.sServers[i].szaIP[2], sizeof(legacy.servers[i].ip3));
            legacy.servers[i].port1 = modern.sServers[i].iaPort[0];
            legacy.servers[i].port2 = modern.sServers[i].iaPort[1];
            legacy.servers[i].port3 = modern.sServers[i].iaPort[2];
        }
        append(&legacy, sizeof(legacy));
    } else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::UserInfo)) {
        if (packet.body.size() != sizeof(PacketUserInfo) - sizeof(PacketHeader)) return std::nullopt;
        PacketUserInfo modern{};
        std::memcpy(reinterpret_cast<uint8_t*>(&modern) + sizeof(PacketHeader), packet.body.data(), packet.body.size());
        struct LegacyCharacter { char name[32], model[64], model2[64]; uint32_t job, level, brood, armor; int start, x, z; uint32_t temp[13]; };
        struct LegacyUsers { int32_t size, code; char id[21]; int32_t count; LegacyCharacter chars[6]; } legacy{};
        legacy.size = sizeof(legacy);
        legacy.code = static_cast<int32_t>(Opcode::SetUserInfo);
        std::memcpy(legacy.id, modern.szUserID, sizeof(legacy.id));
        legacy.count = std::clamp(modern.CharCount, 0, 6);
        for (int i = 0; i < legacy.count; ++i) {
            const auto& src = modern.sCharacterData[i];
            auto& dst = legacy.chars[i];
            std::memcpy(dst.name, src.szName, sizeof(dst.name));
            std::memcpy(dst.model, src.szModelName, sizeof(dst.model));
            std::memcpy(dst.model2, src.szModelName2, sizeof(dst.model2));
            dst.job = src.JobCode; dst.level = src.iLevel; dst.brood = src.Brood; dst.armor = src.dwArmorCode;
            dst.start = src.StartField; dst.x = src.PosX; dst.z = src.PosZ;
            std::memcpy(dst.temp, src.dwTemp, sizeof(dst.temp));
        }
        append(&legacy, sizeof(legacy));
    } else if (packet.header.opcode == static_cast<uint32_t>(Opcodes::RecordResult) ||
               packet.header.opcode == static_cast<uint32_t>(Opcodes::FailRecordData)) {
        if (packet.body.size() < sizeof(PacketRecordResult) - sizeof(PacketHeader)) return std::nullopt;
        PacketRecordResult result{};
        std::memcpy(reinterpret_cast<uint8_t*>(&result) + sizeof(PacketHeader), packet.body.data(),
            sizeof(result) - sizeof(PacketHeader));
        if (packet.header.opcode == static_cast<uint32_t>(Opcodes::RecordResult) && requestCode == 5 &&
            result.Result != 0 && packet.body.size() >= sizeof(PacketRecordResult) - sizeof(PacketHeader) + sizeof(CharacterData)) {
            CharacterData character{};
            std::memcpy(&character, packet.body.data() + sizeof(PacketRecordResult) - sizeof(PacketHeader), sizeof(character));
            return characterRecordPayload(character);
        }
        const int32_t responseCode = requestCode == 10 || requestCode == 11
            ? requestCode
            : static_cast<int32_t>(packet.header.opcode == static_cast<uint32_t>(Opcodes::RecordResult) ? 7 : 6);
        Command legacy{sizeof(Command), responseCode,
            0, result.Result, 0, 0};
        append(&legacy, sizeof(legacy));
    } else {
        return std::nullopt;
    }
    if (payload.empty()) return payload;
    return payload;
}

std::vector<uint8_t> gameServerReadyPayload() {
    // The original SrcServer sends VERSION immediately after a GameServer
    // connection. SERVER_INFO belongs to the login/server-selection stage.
    constexpr int32_t legacyClientVersion = 4135;
    Command legacy{sizeof(Command), static_cast<int32_t>(Opcode::Version), 0, legacyClientVersion, 0, 0};
    const auto* bytes = reinterpret_cast<const uint8_t*>(&legacy);
    return std::vector<uint8_t>(bytes, bytes + sizeof(legacy));
}

} // namespace LegacyGateway
