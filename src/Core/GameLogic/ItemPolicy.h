#pragma once

#include "Shared/datatypes.h"
#include <optional>

class DatabasePool;

class ItemPolicy {
public:
    explicit ItemPolicy(DatabasePool& database) : m_database(database) {}
    bool canEquip(int itemCode, int characterLevel, int characterClass, int equipmentSlot) const;
    bool canUse(int itemCode) const;

private:
    DatabasePool& m_database;
};
