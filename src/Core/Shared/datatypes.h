#pragma once
#include <cstdint>

#pragma pack(push, 1)

// Exemplo da MINIMA estrutura de um item para compilar.
// PREENCHA com os membros corretos do seu servidor.
struct Item {
    int32_t     sItemID;
    char        szItemName[64];
    // ... adicione outros campos ...
};

// Exemplo da MINIMA estrutura de um personagem para compilar.
// PREENCHA com os membros corretos do seu servidor.
struct CharacterData {
    char        szName[32];
    int32_t     iLevel;
    int64_t     liEXP;
    // ... adicione outros campos ...
};

// Exemplo da MINIMA estrutura do depósito para compilar.
// PREENCHA com os membros corretos do seu servidor.
struct WarehouseData {
    int32_t     iGold;
    Item        saItem[100]; // Reduzido para compilar mais rápido por enquanto
};

// Exemplo da MINIMA estrutura de uma definição de item para compilar.
// PREENCHA com os membros corretos do seu servidor.
struct DefinitionItem {
    Item sItem;
    int AttackRatingMin;
    int AttackRatingMax;
    // ... adicione outros campos ...
};

#pragma pack(pop)