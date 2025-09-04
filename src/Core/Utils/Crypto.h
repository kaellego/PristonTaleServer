#pragma once

#include <string>
#include "Utils/picosha2.h" // Add this header to your Utils folder

namespace Crypto {

    // Hashes a password string using SHA-256 and returns the hex string.
    inline std::string hashPassword(const std::string& password) {
        return picosha2::hash256_hex_string(password);
    }

    // Validates a plaintext password against a stored hash.
    inline bool validatePassword(const std::string& plainPassword, const std::string& hashedPassword) {
        // First, check for legacy plaintext passwords for compatibility
        if (plainPassword == hashedPassword) {
            return true;
        }
        // Then, check against the SHA-256 hash
        return hashPassword(plainPassword) == hashedPassword;
    }

} // namespace Crypto