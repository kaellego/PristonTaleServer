#pragma once

#include <algorithm>
#include <cstring>
#include <string_view>

// Copies into a fixed legacy field without invoking the CRT invalid-parameter
// handler when the source is longer than the destination.
template <std::size_t Capacity>
void copyFixed(char (&destination)[Capacity], std::string_view source) noexcept
{
    static_assert(Capacity > 0);
    const auto count = std::min(source.size(), Capacity - 1);
    if (count != 0) std::memcpy(destination, source.data(), count);
    destination[count] = '\0';
}

template <std::size_t Capacity>
void copyFixed(char (&destination)[Capacity], const char* source) noexcept
{
    copyFixed(destination, source ? std::string_view(source) : std::string_view{});
}

