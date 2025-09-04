#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Shared/datatypes.h" // <-- Inclui a definição de Item e DefinitionItem

// Forward declaration
class DatabaseManager;
class LogService;

class ItemRepository {
public:
    explicit ItemRepository(DatabaseManager& dbManager, LogService& logService);

    // --- MÉTODOS CORRIGIDOS ---
    const DefinitionItem* findItemDef(int itemCode) const;
    const DefinitionItem* findOldItemDef(int itemCode) const;

    bool updateItemStats(Item& item) const;
    void stripAugments(Item& item) const;
    void applyAugments(const Item& originalItem, Item& updatedItem) const;

    template<typename T>
    bool adjustStat(const std::string& itemName, T& statValue, T newMin, T newMax, T oldMin, T oldMax) const;

private:
    void loadItemDefinitions();

    DatabaseManager& m_dbManager;
    LogService& m_logService;
    std::map<int, DefinitionItem> m_itemDefs;
    std::map<int, DefinitionItem> m_oldItemDefs;
};