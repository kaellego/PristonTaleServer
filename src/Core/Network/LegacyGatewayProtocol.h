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
    GetUserInfo = 8,
    SetUserInfo = 9,
    FailConnect = 0x22,
    ServerInfo = 0x23,
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
bool isSafeString(const char* value, size_t capacity);

} // namespace LegacyGateway
