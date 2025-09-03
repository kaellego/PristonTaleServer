#pragma once

#include <cstdint>
#include <windows.h>
#include "Network/Packet.h" // <-- A CORREÇÃO PRINCIPAL: Inclui a definição única de Packet.h

#pragma pack(push, 1)

// --- Opcodes e Constantes Globais ---
// Adicionamos aqui os identificadores que estavam faltando
constexpr uint16_t PKTHDR_AccountLoginCode = 0x01E2;


// --- Enums do Jogo ---
// Adicionamos o enum que estava faltando
enum class EBanStatus {
    BANSTATUS_NotBanned,
    BANSTATUS_Banned,
    BANSTATUS_TempBanned,
    BANSTATUS_BanExpired
};

// --- Estruturas de Pacotes de Rede ---
// A struct PacketHeader foi REMOVIDA daqui.

struct PacketLoginUser {
    PacketHeader header;
    char         szUserID[32];
    char         szPassword[32];
    uint32_t     dwSerialHD;
    char         szMacAddr[20];
    char         szPCName[32];
    char         szVideoName[70];
    char         szHardwareID[40];
    uint16_t     uWidthScreen;
    uint16_t     uHeightScreen;
    uint16_t     iSystemOS;
};

struct PacketAccountLoginCode {
    PacketHeader header;
    uint32_t     dwReserved;
    int32_t      iCode;
    int32_t      iFailCode;
    char         szMessage[128];
};

// --- Estruturas de Dados do Jogo ---
struct SQLUser {
    int32_t     iID;
    char        szAccountName[32];
    char        szPassword[65]; // Para acomodar hashes de senha
    int32_t     iFlag;
    int32_t     iActive;
    int32_t     iCoins;
    int32_t     bGameMasterType;
    int32_t     iGameMasterLevel;
    char        szGameMasterMacAddress[20];
    int32_t     iBanStatus;
    SYSTEMTIME  sUnbanDate;
    int32_t     bIsMuted;
    int32_t     iMuteCount;
    SYSTEMTIME  sUnmuteDate;
};

// Estrutura de um item, como salvo em arquivo ou enviado pela rede.
struct Item {
    int32_t     sItemID;
    char        szItemName[64];
    // ... preencha com todos os outros membros exatos do item
};

// Estrutura de um personagem, como salvo no arquivo.chr.
struct CharacterData {
    char        szName[32];
    int32_t     iLevel;
    int64_t     liEXP;
    // ... preencha com todos os outros membros exatos do personagem
};

// Estrutura do depósito, como salvo no arquivo .dat.
struct WarehouseData {
    int32_t     iGold;
    Item        saItem[405];
    // ... preencha com outros membros
};

// Estrutura para uma definição de item carregada da tabela ItemList do banco de dados.
struct DefinitionItem {
    Item sItem;
    int AttackRatingMin;
    int AttackRatingMax;
    // ... preencha com outros membros
};
#pragma pack(pop)