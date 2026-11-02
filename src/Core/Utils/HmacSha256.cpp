#include "Utils/HmacSha256.h"

#include <Windows.h>
#include <bcrypt.h>
#include <limits>

#pragma comment(lib, "bcrypt.lib")

namespace HmacSha256 {
std::optional<std::array<uint8_t, 32>> calculate(std::string_view secret, const std::vector<uint8_t>& data) {
    if (secret.empty() || secret.size() > std::numeric_limits<ULONG>::max() || data.size() > std::numeric_limits<ULONG>::max())
        return std::nullopt;

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
        return std::nullopt;

    DWORD objectLength = 0;
    DWORD returned = 0;
    const auto propertyStatus = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &returned, 0);
    if (propertyStatus != 0 || objectLength == 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return std::nullopt;
    }

    std::vector<uint8_t> object(objectLength);
    BCRYPT_HASH_HANDLE hash = nullptr;
    const auto createStatus = BCryptCreateHash(algorithm, &hash, object.data(), objectLength,
        reinterpret_cast<PUCHAR>(const_cast<char*>(secret.data())), static_cast<ULONG>(secret.size()), 0);
    if (createStatus != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return std::nullopt;
    }

    const auto dataStatus = BCryptHashData(hash, data.empty() ? nullptr : const_cast<PUCHAR>(data.data()),
        static_cast<ULONG>(data.size()), 0);
    std::array<uint8_t, 32> result{};
    const auto finishStatus = dataStatus == 0 ? BCryptFinishHash(hash, result.data(), static_cast<ULONG>(result.size()), 0) : -1;
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return finishStatus == 0 ? std::optional<std::array<uint8_t, 32>>(result) : std::nullopt;
}

bool constantTimeEqual(const std::vector<uint8_t>& actual, const std::array<uint8_t, 32>& expected) {
    uint8_t difference = actual.size() == expected.size() ? 0 : 1;
    for (std::size_t i = 0; i < expected.size(); ++i)
        difference |= static_cast<uint8_t>(actual.size() == expected.size() ? actual[i] ^ expected[i] : 0);
    return difference == 0;
}
}
