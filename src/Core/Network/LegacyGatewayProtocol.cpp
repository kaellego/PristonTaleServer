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
    struct ServerInfo { char name[32], ip1[20], ip2[20], ip3[20]; uint32_t port1, port2, port3, state; };
    struct ServerList { int32_t size, code; char name[16]; uint32_t time; int32_t clanTicket, temp, clanCount, count; ServerInfo servers[8]; } legacy{};
    legacy.size = sizeof(legacy);
    legacy.code = static_cast<int32_t>(Opcode::ServerInfo);
    std::memcpy(legacy.name, "Awell", sizeof("Awell"));
    legacy.count = 1;
    std::memcpy(legacy.servers[0].name, "Awell", sizeof("Awell"));
    std::memcpy(legacy.servers[0].ip1, "127.0.0.1", sizeof("127.0.0.1"));
    std::memcpy(legacy.servers[0].ip2, "127.0.0.1", sizeof("127.0.0.1"));
    std::memcpy(legacy.servers[0].ip3, "127.0.0.1", sizeof("127.0.0.1"));
    legacy.servers[0].port1 = legacy.servers[0].port2 = legacy.servers[0].port3 = 5056;
    const auto* bytes = reinterpret_cast<const uint8_t*>(&legacy);
    return std::vector<uint8_t>(bytes, bytes + sizeof(legacy));
}

} // namespace LegacyGateway
