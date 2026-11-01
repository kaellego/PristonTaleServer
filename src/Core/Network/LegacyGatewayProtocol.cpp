#include "Network/LegacyGatewayProtocol.h"
#include "Shared/Constants.h"
#include "Shared/datatypes.h"

#include <cstring>

namespace LegacyGateway {

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
    case Opcode::GetUserInfo:
        return loginToModernPacket(payload);
    default:
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

} // namespace LegacyGateway
