#pragma once

#include "Shared/datatypes.h"
#include <atomic>
#include <cstdint>
#include <string>

namespace ItemIntegrity {
    inline std::string& secret() { static std::string value; return value; }
    inline void setSecret(std::string value) { secret() = std::move(value); }
    inline uint64_t nextSerial() {
        static std::atomic_uint64_t counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
    inline uint32_t checksum(const Item& item) {
        uint64_t value = 1469598103934665603ull;
        for (const unsigned char byte : secret()) { value ^= byte; value *= 1099511628211ull; }
        value ^= static_cast<uint32_t>(item.sItemID); value *= 1099511628211ull;
        value ^= static_cast<uint32_t>(item.iQuantity); value *= 1099511628211ull;
        value ^= item.serial; value *= 1099511628211ull;
        return static_cast<uint32_t>((value ^ (value >> 32)) & 0xffffffffu);
    }
    inline void initialize(Item& item) {
        if (item.sItemID <= 0) { item = {}; return; }
        if (item.iQuantity <= 0) item.iQuantity = 1;
        if (item.serial == 0) item.serial = nextSerial();
        item.version = item.version == 0 ? 1 : item.version;
        item.checksum = checksum(item);
    }
    inline bool valid(const Item& item) {
        return item.sItemID > 0 && item.iQuantity > 0 && item.serial != 0 && item.version > 0 && item.checksum == checksum(item);
    }
}
