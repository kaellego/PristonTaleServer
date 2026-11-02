#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace HmacSha256 {
std::optional<std::array<uint8_t, 32>> calculate(std::string_view secret, const std::vector<uint8_t>& data);
bool constantTimeEqual(const std::vector<uint8_t>& actual, const std::array<uint8_t, 32>& expected);
}
