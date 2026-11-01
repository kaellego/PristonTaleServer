#pragma once

#include "Network/Packet.h"

#include <cstdint>
#include <optional>
#include <vector>

class LegacyPacketAdapter final {
public:
    std::optional<Packet> decode(const std::vector<uint8_t>& payload) const;
    static bool validFrame(const std::vector<uint8_t>& payload);
};
