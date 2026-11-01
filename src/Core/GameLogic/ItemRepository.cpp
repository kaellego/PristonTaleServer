#include "GameLogic/ItemRepository.h"
#include "Database/DatabasePool.h"
#include "Logging/LogService.h"
#include "Database/SQLConnection.h"
#include "Utils/Dice.h"
#include <iostream>
#include <cmath>

ItemRepository::ItemRepository(DatabasePool& dbPool, LogService& logService)
    : m_dbPool(dbPool),
    m_logService(logService)
{
    m_logService.info("Inicializando repositorio de itens...");
    loadItemDefinitions();
}

void ItemRepository::loadItemDefinitions() {
    auto db = m_dbPool.createConnection(EDatabaseID::GameDB); db->prepare("SELECT ItemCode, ItemName FROM ItemList"); db->execute();
    while (db->fetch()) {
        auto code = db->getData<int>(1).value_or(0); auto name = db->getData<std::string>(2).value_or("");
        if (code <= 0 || name.empty()) continue;
        DefinitionItem definition{};
        definition.sItem.sItemID = code;
        strncpy_s(definition.sItem.szItemName, sizeof(definition.sItem.szItemName), name.c_str(), _TRUNCATE);
        m_itemDefs.emplace(code, definition);
    }
    m_logService.info("{} definicoes de itens carregadas.", m_itemDefs.size());
}

// --- DEFINIÇÕES DE MÉTODOS CORRIGIDAS ---

const DefinitionItem* ItemRepository::findItemDef(int itemCode) const {
    auto it = m_itemDefs.find(itemCode);
    return (it != m_itemDefs.end()) ? &it->second : nullptr;
}

const DefinitionItem* ItemRepository::findOldItemDef(int itemCode) const {
    auto it = m_oldItemDefs.find(itemCode);
    return (it != m_oldItemDefs.end()) ? &it->second : nullptr;
}

bool ItemRepository::updateItemStats(Item& item) const {
    const auto* definition = findItemDef(item.sItemID);
    if (!definition) return false;
    std::memcpy(&item, &definition->sItem, sizeof(Item));
    return true;
}

void ItemRepository::stripAugments(Item& item) const {
    (void)item;
}

void ItemRepository::applyAugments(const Item& originalItem, Item& updatedItem) const {
    updatedItem = originalItem;
}

template<typename T>
bool ItemRepository::adjustStat(const std::string& itemName, T& statValue, T newMin, T newMax, T oldMin, T oldMax) const {
    (void)itemName;
    if (newMin > newMax || oldMin > oldMax || statValue < oldMin || statValue > oldMax) return false;
    const auto oldRange = oldMax - oldMin;
    if (oldRange == T{}) { statValue = newMin; return true; }
    const auto ratio = static_cast<double>(statValue - oldMin) / static_cast<double>(oldRange);
    statValue = static_cast<T>(newMin + (newMax - newMin) * ratio);
    return true;
}

// Necessário para o compilador encontrar a implementação do template
template bool ItemRepository::adjustStat<int>(const std::string&, int&, int, int, int, int) const;
template bool ItemRepository::adjustStat<short>(const std::string&, short&, short, short, short, short) const;
template bool ItemRepository::adjustStat<float>(const std::string&, float&, float, float, float, float) const;
