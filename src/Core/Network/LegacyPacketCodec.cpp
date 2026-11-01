#include "Network/LegacyPacketCodec.h"

#include <cstring>

uint16_t LegacyPacketCodec::checksum(const uint8_t* data, size_t size) {
    uint32_t total = 0;
    for (size_t i = 0; i < size; ++i) total += data[i];
    return static_cast<uint16_t>(total & 0xffffu);
}

bool LegacyPacketCodec::decode(const std::vector<uint8_t>& bytes, uint32_t expectedCounter, Frame& frame) {
    if (bytes.size() < HeaderSize) return false;

    uint32_t length = 0, wireChecksum = 0, key = 0;
    std::memcpy(&length, bytes.data(), sizeof(length));
    std::memcpy(&wireChecksum, bytes.data() + 4, sizeof(wireChecksum));
    std::memcpy(&key, bytes.data() + 8, sizeof(key));

    if (length < 12 || length > MaxFrameSize || length != bytes.size()) return false;
    if (expectedCounter != 0 && (key & 0xffffu) <= (expectedCounter & 0xffffu)) return false;

    const uint32_t legacyLength = length - 8;
    if (legacyLength < 4 || legacyLength > MaxFrameSize - 8) return false;
    frame.payload.assign(legacyLength, 0);
    std::memcpy(frame.payload.data(), &legacyLength, sizeof(legacyLength));

    uint32_t rKey = (key >> 4) + legacyLength;
    uint8_t near = static_cast<uint8_t>((key & 0xffffu) + (key >> 16));
    uint16_t total = 0;
    for (uint32_t i = 4; i < legacyLength; ++i) {
        const uint8_t encoded = bytes[i + 8];
        const uint8_t decoded = static_cast<uint8_t>(encoded ^ static_cast<uint8_t>(rKey) ^ near);
        frame.payload[i] = decoded;
        total = static_cast<uint16_t>(total + decoded);
        rKey += i * (i >> 1);
        near = encoded; // EncodePacket variant used by this client has no extra XOR.
    }
    if ((wireChecksum & 0xffffu) != total) return false;

    frame.key = key;
    return true;
}

std::vector<uint8_t> LegacyPacketCodec::encode(const std::vector<uint8_t>& payload, uint32_t key) {
    if (payload.size() < 4) return {};
    uint32_t legacyLength = 0;
    std::memcpy(&legacyLength, payload.data(), sizeof(legacyLength));
    if (legacyLength != payload.size() || legacyLength < 4) return {};
    const uint32_t length = legacyLength + 8;
    if (length > MaxFrameSize) return {};

    std::vector<uint8_t> result(length);
    uint32_t packetCount = key & 0xffffu;
    uint32_t packetKey = key >> 16;
    uint32_t sKey = (packetKey << 16) | packetCount;
    uint32_t rKey = (sKey >> 4) + legacyLength;
    uint8_t near = static_cast<uint8_t>(packetCount + packetKey);
    uint16_t sum = 0;
    for (uint32_t i = 4; i < legacyLength; ++i) {
        const uint8_t decoded = payload[i];
        sum = static_cast<uint16_t>(sum + decoded);
        result[i + 8] = static_cast<uint8_t>(decoded ^ static_cast<uint8_t>(rKey) ^ near);
        rKey += i * (i >> 1);
        near = result[i + 8];
    }
    std::memcpy(result.data(), &length, sizeof(length));
    const uint32_t checksumValue = (packetKey << 16) | sum;
    std::memcpy(result.data() + 4, &checksumValue, sizeof(checksumValue));
    std::memcpy(result.data() + 8, &sKey, sizeof(sKey));
    return result;
}
