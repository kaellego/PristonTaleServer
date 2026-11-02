#include "Network/LegacyPacketAdapter.h"

#include "Network/LegacyGatewayProtocol.h"
#include "Shared/Constants.h"

#include <cstring>

bool LegacyPacketAdapter::validFrame(const std::vector<uint8_t>& payload) {
    // Chat messages are the only supported legacy frame shorter than a full
    // smTRANS_COMMAND: size, opcode, and a null-terminated message.
    if (payload.size() < sizeof(int32_t) * 2 || payload.size() > Constants::MAX_PKTSIZ)
        return false;

    int32_t declaredSize = 0;
    std::memcpy(&declaredSize, payload.data(), sizeof(declaredSize));
    return declaredSize >= static_cast<int32_t>(sizeof(int32_t) * 2) &&
           declaredSize == static_cast<int32_t>(payload.size());
}

std::optional<Packet> LegacyPacketAdapter::decode(const std::vector<uint8_t>& payload) const {
    if (!validFrame(payload))
        return std::nullopt;
    return LegacyGateway::toModernPacket(payload);
}
