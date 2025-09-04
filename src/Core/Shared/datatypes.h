    #pragma once

#include <cstdint>
#include <windows.h>
#include "Network/Packet.h" // <-- A CORREÇÃO PRINCIPAL: Inclui a definição única de Packet.h

#pragma pack(push, 1)

// --- Opcodes e Constantes Globais ---
constexpr uint32_t PKTHDR_AccountLoginCode = 0x48470023;
constexpr uint32_t PKTHDR_UserInfo = 0x48470086;


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

struct PacketLoginUserBody {
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

struct PacketLoginUser {
    PacketHeader header;         // 8 bytes

    // Corpo do Pacote
    uint32_t     dwUnk[3];       // 12 bytes
    char         szUserID[32];
    char         szPassword[65];     // 65 bytes
    char         padding[3];         // <-- A CORREÇÃO: 3 bytes de preenchimento para corresponder ao alinhamento do cliente
    char         szMacAddr[20];
    char         szPCName[32];
    uint32_t     dwSerialHD;
    char         szVideoName[256];
    char         szHardwareID[40];
    uint32_t     uWidthScreen;
    uint32_t     uHeightScreen;
    int32_t      iSystemOS;
    int32_t      iVersion;
};

struct PacketAccountLoginCode {
    PacketHeader header;
    uint32_t     dwReserved;
    int32_t      iCode;
    int32_t      iFailCode;
    char         szMessage[128];
};

struct TransCharInfo
{
    char    szName[32];
    char    szModelName[64];
    char    szModelName2[64];
    int32_t JobCode;
    int32_t iLevel;
    int32_t Brood;
    int32_t dwArmorCode;
    int32_t StartField;
    int32_t PosX, PosZ;
    int32_t dwTemp[13];
};

// Pacote enviado pelo servidor com a lista de personagens.
// Contém até 6 structs TransCharInfo.
struct PacketUserInfo {
    PacketHeader     header;
    char             szUserID[32];
    int32_t          CharCount;
    TransCharInfo    sCharacterData[6];
};

// --- Estruturas de Dados do Jogo ---
struct SQLUser {
    int32_t     iID;
    char        szAccountName[32];
    char        szPassword[65]; // Para acomodar hashes de senha
    int32_t     iFlag;

    EBanStatus  iBanStatus;

    int32_t     iActive;
    int32_t     iCoins;
    int32_t     bGameMasterType;
    int32_t     iGameMasterLevel;
    char        szGameMasterMacAddress[20];
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
    int32_t     iClass;
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

struct PacketVersion {
    PacketHeader header;
    BOOL bServerFull;
    int iVersion;
    int iUnk2;
};

struct PacketAttackData {
    PacketHeader header;
    uint32_t dwDestObjectSerial;
    uint32_t dwTarObjectSerial;
    // ... preencha com o resto dos membros ...
};

struct PacketSingleTargetSkillData {
    PacketHeader header;
    uint32_t dwChkSum;
    // ... preencha com o resto dos membros ...
};

#pragma pack(pop)