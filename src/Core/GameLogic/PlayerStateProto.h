#pragma once

#include "Shared/datatypes.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace PlayerStateProto {
std::vector<uint8_t> encode(const CharacterData& value);
std::vector<uint8_t> encode(const InventoryData& value);
std::vector<uint8_t> encode(const WarehouseData& value);

std::optional<CharacterData> decodeCharacter(const std::vector<uint8_t>& bytes);
std::optional<InventoryData> decodeInventory(const std::vector<uint8_t>& bytes);
std::optional<WarehouseData> decodeWarehouse(const std::vector<uint8_t>& bytes);
}
