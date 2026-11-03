#include "Utils/PasswordVerifier.h"
#include "Utils/picosha2.h"

namespace PasswordVerifier {

bool matches(const std::string& receivedPassword, const std::string& databaseValue) {
    if (isSha256Hex(databaseValue)) {
        return constantTimeEqual(picosha2::hash256_hex_string(receivedPassword), databaseValue);
    }
    return constantTimeEqual(receivedPassword, databaseValue);
}

} // namespace PasswordVerifier
