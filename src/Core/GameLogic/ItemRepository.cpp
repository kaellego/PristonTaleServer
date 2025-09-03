#include "GameLogic/ItemRepository.h"
#include "Database/DatabaseManager.h"
#include "Database/SQLConnection.h"
#include "Shared/datatypes.h"      // <--- Garante que as structs sejam conhecidas
#include "Utils/Dice.h"            // <--- Inclui a classe de dados que faltava
#include <iostream>
#include <cmath>

ItemRepository::ItemRepository(DatabaseManager& dbManager) : m_dbManager(dbManager) {
    std::cout << "Inicializando repositorio de itens..." << std::endl;
    // loadItemDefinitions(); // Vamos desativar o carregamento por enquanto para compilar
}

void ItemRepository::loadItemDefinitions() {
    // A implementação de carregamento do DB vai aqui...
}

const DefinitionItem* ItemRepository::findItemDef(int itemCode) const {
    auto it = m_itemDefs.find(itemCode);
    return (it != m_itemDefs.end()) ? &it->second : nullptr;
}

const DefinitionItem* ItemRepository::findOldItemDef(int itemCode) const {
    auto it = m_oldItemDefs.find(itemCode);
    return (it != m_oldItemDefs.end()) ? &it->second : nullptr;
}

bool ItemRepository::updateItemStats(Item& item) const {
    // A lógica de atualização vai aqui. Por enquanto, retornamos false.
    return false;
}

void ItemRepository::stripAugments(Item& item) const {
    // Lógica para remover bônus...
}

void ItemRepository::applyAugments(const Item& originalItem, Item& updatedItem) const {
    // Lógica para reaplicar bônus...
}

// A implementação do template permanece no arquivo .tpp ou no .h