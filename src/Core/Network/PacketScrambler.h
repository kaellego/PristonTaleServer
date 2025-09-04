#pragma once
#include <cstdint>
#include <vector>

namespace PacketScrambler {

    // A função agora recebe a chave como um argumento
    inline void scramble(uint8_t* data, size_t length, uint8_t xorKey) {
        for (size_t i = 0; i < length; ++i) {
            data[i] ^= xorKey;
        }
    }

    inline void scramble(std::vector<uint8_t>& buffer, uint8_t xorKey) {
        scramble(buffer.data(), buffer.size(), xorKey);
    }

    template<typename T>
    void scramble(T& structure, uint8_t xorKey) {
        scramble(reinterpret_cast<uint8_t*>(&structure), sizeof(T), xorKey);
    }

} // namespace PacketScrambler