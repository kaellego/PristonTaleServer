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
    // A implementa��o de carregamento do DB vai aqui...
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
    // A l�gica de atualiza��o vai aqui. Por enquanto, retornamos false.
    return false;
}

void ItemRepository::stripAugments(Item& item) const {
    // L�gica para remover b�nus...
}

void ItemRepository::applyAugments(const Item& originalItem, Item& updatedItem) const {
    // L�gica para reaplicar b�nus...
}

// A implementa��o do template permanece no arquivo .tpp ou no .h