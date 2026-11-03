#pragma once

#include <string>
#include <string_view>
#include <cctype>

namespace PasswordVerifier {

inline bool constantTimeEqual(std::string_view left, std::string_view right) {
    const size_t length = left.size() > right.size() ? left.size() : right.size();
    unsigned char difference = static_cast<unsigned char>(left.size() ^ right.size());
    for (size_t index = 0; index < length; ++index) {
        const unsigned char leftByte = index < left.size()
            ? static_cast<unsigned char>(left[index]) : 0;
        const unsigned char rightByte = index < right.size()
            ? static_cast<unsigned char>(right[index]) : 0;
        difference = static_cast<unsigned char>(difference | (leftByte ^ rightByte));
    }
    return difference == 0;
}

inline bool isSha256Hex(std::string_view value) {
    if (value.size() != 64)
        return false;
    for (const unsigned char byte : value) {
        if (!std::isxdigit(byte))
            return false;
    }
    return true;
}

bool matches(const std::string& receivedPassword, const std::string& databaseValue);

} // namespace PasswordVerifier
