#pragma once

#include <cstdint>
#include <vector>

// Transporte usado pelo cliente legado: [length][checksum][key/counter][payload].
// O codec rejeita frames inválidos antes que alcancem os handlers.
class LegacyPacketCodec {
public:
    static constexpr uint32_t HeaderSize = 12;
    static constexpr uint32_t MaxFrameSize = 8192;

    struct Frame {
        uint32_t key = 0;
        std::vector<uint8_t> payload;
    };

    static bool decode(const std::vector<uint8_t>& bytes, uint32_t expectedCounter, Frame& frame);
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& payload, uint32_t key);

private:
    static uint16_t checksum(const uint8_t* data, size_t size);
};
