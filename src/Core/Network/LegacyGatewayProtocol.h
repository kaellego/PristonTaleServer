#pragma once

#include "Network/Packet.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace LegacyGateway {

// Values from Shared/smPacket.h. They are kept explicit so a legacy value
// cannot accidentally be treated as a modern opcode.
enum class Opcode : int32_t {
    Connected = 3,
    RecordData = 4,
    PlayData1 = 28,
    PlayDataGroup = 32,
    SetBlacklist = 39,
    PlayerInfo = 62,
    GetPlayerInfo = 63,
    AddNpc = 88,
    TransPlayData = 31,
    ProcessInfo = 106,
    NetworkQuality = 105,
    GetUserInfo = 8,
    SetUserInfo = 9,
    Version = 13,
    ChatMessage = 22,
    FailConnect = 64,
    CheckIP = 108,
    FunctionResult = 187,
    ServerInfo = 96,
    EffectLowPet = 136,
    AttackData = 36,
    AttackDamage = 212,
    AttackResult = 214,
    DispDamage = 215,
    AttackData2 = 216,
    ChatMessageLegacy = 22,
};

#pragma pack(push, 1)
struct Command {
    int32_t size;
    int32_t code;
    int32_t lParam;
    int32_t wParam;
    int32_t sParam;
    int32_t eParam;
};

struct CharacterCommand {
    int32_t size;
    int32_t code;
    int32_t lParam;
    int32_t wParam;
    int32_t sParam;
    char id[21];
    char name[32];
};

struct LoginPacket {
    int32_t size;
    int32_t code;
    char account[21];
    char password[32];
    char hash[33];
    char mac[30];
};
#pragma pack(pop)

std::optional<Packet> toModernPacket(const std::vector<uint8_t>& payload);
std::optional<Packet> loginToModernPacket(const std::vector<uint8_t>& payload);
std::optional<std::vector<uint8_t>> toLegacyPayload(const Packet& packet, uint32_t packetKey, int32_t requestCode = 0);
std::vector<uint8_t> gameServerReadyPayload();
bool isSafeString(const char* value, size_t capacity);

} // namespace LegacyGateway
