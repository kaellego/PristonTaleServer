#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Constants {

    // --- Configurações de Rede e Pacotes ---
    constexpr int MAX_PKTSIZ = 0x2000;
    constexpr int PKTHDR_ENCMAX = 0x10;

    // --- Configurações de Jogo ---
    constexpr int MAX_SKILL_ARRAY_DATA = 150;
    constexpr int MAX_COMMON_SKILL_INFO_DATA = 25;
    constexpr int SLOW_SPEED_BASE = 256;
    constexpr int SERVER_CHAR_FRAME_STEP = 320;
    constexpr int SERVER_CHAR_SKIP = 4;
    constexpr int MAX_CONTINUE_SKILL = 20;
    constexpr int BURNING_TICKRATE_MS = 500;
    constexpr int POISON_MAXSTACK = 5;
    constexpr int POISON_DURATION_SEC = 30;
    constexpr int POISON_TICKRATE_MS = 1000;

    //See https://docs.google.com/document/d/1V9Ozk5gN-RaBpgflo-8_1zZzde2h5y5np4n8r1t2Q9w/edit
    // --- Constantes de Distância (Exemplo) ---
    // Usar static constexpr torna os valores type-safe e com escopo.
    static constexpr int DISTANCE_XY_152_meters = 16777216; // (152 meters)  (((64*64)+(64*64)) * 2048)
    static constexpr int DISTANCE_XY_107_meters = 8388608; // (107 meters)  (((64*64)+(64*64)) * 1024)
    static constexpr int DISTANCE_XY_76_meters = 4194304; // ( 76 meters)  (((64*64)+(64*64)) *  512)
    static constexpr int DISTANCE_XY_60_meters = 2621440; // ( 60 meters)  (((64*64)+(64*64)) *  320)
    static constexpr int DISTANCE_XY_54_meters = 2097152; // ( 54 meters)  (((64*64)+(64*64)) *  256)
    static constexpr int DISTANCE_XY_47_meters = 1638400; // ( 47 meters)  (((64*64)+(64*64)) *  200) iLureDistance uses this distance
    static constexpr int DISTANCE_XY_41_meters = 1228800; // ( 41 meters)  (((64*64)+(64*64)) *  150)
    static constexpr int DISTANCE_XY_38_meters = 1048576; // ( 38 meters)  (((64*64)+(64*64)) *  128)
    static constexpr int DISTANCE_XY_33_meters = 819200; // ( 33 meters)  (((64*64)+(64*64)) *  100)
    static constexpr int DISTANCE_XY_30_meters = 640000; // ( 30 meters)
    static constexpr int DISTANCE_XY_27_meters = 524288; // ( 27 meters)  (((64*64)+(64*64)) *  64)
    static constexpr int DISTANCE_XY_24_meters = 409600; // ( 24 meters)  (((64*64)+(64*64)) *  50)
    static constexpr int DISTANCE_XY_20_meters = 291600; // ( 20 meters)
    static constexpr int DISTANCE_XY_19_meters = 262144; // ( 19 meters)  (((64*64)+(64*64)) *  32)
    static constexpr int DISTANCE_XY_15_meters = 164025; // ( 15 meters)
    static constexpr int DISTANCE_XY_13_meters = 131072; // ( 13 meters)  (((64*64)+(64*64)) *  16)
    static constexpr int DISTANCE_XY_10_meters = 65536; // ( 10 meters)  (((64*64)+(64*64)) *  8)
    static constexpr int DISTANCE_XY_7_meters = 32768; // ( 7 meters)   (((64*64)+(64*64)) *  4)
    static constexpr int DISTANCE_XY_5_meters = 16384; // ( 5 meters)   (((64*64)+(64*64)) *  2)
    static constexpr int DISTANCE_XY_3_meters = 8192; // ( 3 meters)   (((64*64)+(64*64)) *  1)
    static constexpr int DISTANCE_XY_05 = 4096; // ( 1.5 meters) (((64*64)+(64*64)) *  0.5)
    static constexpr int DISTANCE_XY_025 = 2048; // ( 0.7 meters) (((64*64)+(64*64)) *  0.25)

    static constexpr int DISTANCE_300 = 90000; //~11m

    static constexpr int DISTANCE_MAX_UNIT_VIEWLIMIT = DISTANCE_XY_107_meters;; //107 meters

    static constexpr int DISTANCE_MAX_UNIT_OUTOFRANGE = DISTANCE_XY_60_meters;	//60 meters
    static constexpr int DISTANCE_MAX_PLAYER_BASIC_VIEW = DISTANCE_XY_54_meters;	//54 meters
    static constexpr int DISTANCE_MAX_UNIT_BASIC_VIEW = DISTANCE_XY_54_meters;	//54 meters
    static constexpr int DISTANCE_MAX_PARTY = DISTANCE_XY_41_meters;	//41 meters
    static constexpr int DISTANCE_MAX_CHATRANGE = DISTANCE_XY_41_meters;	//41 meters
    static constexpr int DISTANCE_MAX_PLAYER_DETAILED_VIEW = DISTANCE_XY_33_meters;	//33 meters
    static constexpr int DISTANCE_MAX_UNIT_DETAILED_VIEW = DISTANCE_XY_33_meters;	//33 meters
    static constexpr int DISTANCE_MAX_PLAYER_BASIC_VIEW_RICT = DISTANCE_XY_33_meters;	//33 meters
    static constexpr int DISTANCE_MAX_SKILL_VISUAL = DISTANCE_XY_24_meters;	//24 meters
    static constexpr int DISTANCE_MAX_PET_RANGE = DISTANCE_XY_24_meters;	//24 meters
    static constexpr int DISTANCE_MAX_PLAYER_DETAILED_VIEW_RICT = DISTANCE_XY_24_meters;	//24 meters
    static constexpr int DISTANCE_MAX_BOSS = DISTANCE_XY_24_meters;	//24 meters
    static constexpr int DISTANCE_MAX_SKILL_RANGE = DISTANCE_XY_24_meters;	//24 meters
    static constexpr int DISTANCE_MAX_FROZENSANCTUARY_TREE = DISTANCE_XY_19_meters;	//13 meters

    static constexpr int DISTANCE_MISC = 0x1000; //4096
    static constexpr int DISTANCE_MISC_Y = 300;
    static constexpr int DISTANCE_MISC_Y_EX = 1000;

    constexpr int SERVER_LEVEL_MAX = 120;
    constexpr int MAX_UNIT_PLAYDATA = 75;
    constexpr int MAX_QUESTPACKETDATA = 15;

} // namespace Constants

//Este arquivo conterá todos os enums, convertidos para o formato moderno enum class. 
//Isso os torna mais seguros e fáceis de usar, sem poluir o escopo global.

namespace GameData {

    enum class CharacterType : int {
        None = 0,
        NPC = 0x00,
        Monster = 0x01,
        Player = 0x80 //smCHAR_STATE_PLAYER
    };

    enum class CharacterClass : int {
        None = 0, Fighter = 1, Mechanician = 2, Archer = 3, Pikeman = 4, Atalanta = 5,
        Knight = 6, Magician = 7, Priestess = 8, Assassin = 9, Shaman = 10,
    };

    enum class MonsterType : int {
        Normal = 0x00, Undead = 0x90, Mutant = 0x91, Demon = 0x92,
        Neutral = 0x94, Summon = 0xA0, Pet = 0xA1
    };

    enum class AttackProperty : short {
        Unknown = -1, Undefined = 0, Organic = 1, Earth = 2, Fire = 3, Ice = 4,
        Lightning = 5, Poison = 6, Water = 7, Wind = 8, Holy = 9, None = 15
    };

    enum class ItemFlag : uint8_t {
        None = 0,
        ForceAllowDrop = 1 << 0,
        TreasureHuntingMode = 1 << 1,
    };

    enum class EClassFlag : int
    {
        CLASSFLAG_Unknown = -1,
        CLASSFLAG_None = 0,
        CLASSFLAG_Fighter = 2,
        CLASSFLAG_Mechanician = 1,
        CLASSFLAG_Archer = 4,
        CLASSFLAG_Pikeman = 3,
        CLASSFLAG_Atalanta = 0x00020000,
        CLASSFLAG_Knight = 0x00010000,
        CLASSFLAG_Magician = 0x00040000,
        CLASSFLAG_Priestess = 0x00030000,
        CLASSFLAG_Assassin = 0x00050100,
        CLASSFLAG_Shaman = 0x00050000,
    };

    enum class EAllowedClassSpecFlag : int
    {
        None = 0,

        Fighter = 1 << 0,
        Mechanician = 1 << 1,
        Archer = 1 << 2,
        Pikeman = 1 << 3,
        Assassin = 1 << 4,

        Atalanta = 1 << 8,
        Knight = 1 << 9,
        Magician = 1 << 10,
        Priestess = 1 << 11,
        Shaman = 1 << 12
    };
    
}

/**
 * @enum class Opcodes
 * @brief Enum fortemente tipado para todos os opcodes de pacotes do servidor.
 * * Substitui o antigo enum global 'PacketsHeader', fornecendo segurança de tipos e escopo.
 * Para usar em uma instrução switch, faça o cast do opcode do pacote:
 * switch (static_cast<Opcodes>(packet.header.opcode)) { ... }
 */
enum class Opcodes : uint32_t {
    KeepAlive = 0x00,
    
    // =================================================================================
    // Net Server (Comunicação Inter-servidores)
    // =================================================================================
    NetIdentifier = 0x7F000001,
    NetExp = 0x7F000002,
    NetGetExp = 0x7F000003,
    NetGameMaster = 0x7F000004,
    NetGetGameMaster = 0x7F000005,
    NetClan = 0x7F000006,
    NetDisconnectUser = 0x7F000007,
    NetForceOrb = 0x7F000008,
    NetVersion = 0x7F000009,
    NetCharacterSync = 0x7F00000A,
    NetGiveExp = 0x7F00000B,
    NetPingPong = 0x7F00000C,
    NetBCStatusReq = 0x7F00000D,
    NetBCStatusSnd = 0x7F00000E,
    NetBCNewOwnerClanID = 0x7F00000F,
    NetBotAdd = 0x7F000010,
    NetPremiumEXP = 0x7F000011,
    NetUnloadUser = 0x7F000012,
    NetEXPEvent = 0x7F000013,
    NetTradeCoin = 0x7F000014,
    NetUsersOnline = 0x7F000015,
    NetQuestT5Data = 0x7F000016,
    NetQuestUpdateData = 0x7F000017,
    NetQuestUpdateDataPart = 0x7F000018,
    NetQuestCancelOrFinish = 0x7F000019,
    NetLoadPVPData = 0x7F000020,
    NetLoadQuestData = 0x7F000021,
    NetSyncFinishedQuestData = 0x7F000022,
    NetPlayerWorldToken = 0x7F000040,
    NetPlayerInventory = 0x7F000041,
    NetPlayerGold = 0x7F000042,
    NetPlayerGoldDiff = 0x7F000043,
    NetPlayerThrow = 0x7F000044,
    NetStartMaintenance = 0x7F000045,
    NetPlayerItemPut = 0x7F000046,
    NetEasterEvent = 0x7F000047,
    NetAgingEvent = 0x7F000048,
    NetBellatraSoloRanking = 0x7F000049,
    NetHallowenEvent = 0x7F000050,
    NetOpenWarehouse = 0x7F000051,
    NetOpenWarehouseSeasonal = 0x7F000052,
    NetChristmasEvent = 0x7F000053,
    NetFreeEventGirl = 0x7F000054,
    NetStopMaintenance = 0x7F000056,
    NetLevelUp = 0x7F000058,
    NetSetVersion = 0x7F000059,
    NetReloadCoinShop = 0x7F000060,
    NetClearTickRO = 0x7F000061,
    NetClearTickChristmas = 0x7F000062,
    NetKickAll = 0x7F000063,
    NetMuteSync = 0x7F000064,
    NetGetMuteSync = 0x7F000065,
    NetStarWarsEvent = 0x7F000066,
    NetBeeEvent = 0x7F000067,
    NetOpenItemDistributor = 0x7F000068,
    NetGameMasterCommand = 0x7F000069,
    NetPlayDataEx = 0x7F000070,
    NetRemoveFinishedQuestData = 0x7F000071,
    NetSendItemData = 0x7F000072,
    NetMimicEvent = 0x7F000073,

    // =================================================================================
    // Login & Seleção de Personagem
    // =================================================================================
    LoginUser = 0x48480001,
    AccountLoginCode = 0x48470023,
    Version = 0x4847008A,
    ServerList = 0x484700C0,
    UserInfo = 0x48470086,
    SelectCharacter = 0x48470082,
    CreateCharacter = 0x48470088,
    DeleteCharacter = 0x48470087,
    SaveData = 0x48470081,
    FailRecordData = 0x48470083,
    RecordResult = 0x48470084,

    // =================================================================================
    // Sistema & Conexão
    // =================================================================================
    KeySet = 0x4847FFFF,
    Ping = 0x435A0007,
    PingLoginServer = 0x435A0008,
    Connected = 0x48470080,
    Save = 0x484700E8,
    SaveAndClose = 0x435A0011,
    Disconnect = 0x484700E6,
    Reconnect = 0x48478010,
    ReconnectLogin = 0x48478011,
    NetworkQuality = 0x484700E0,
    UpdateServerParam = 0x503220F0,
    ServerInfo = 0x43550005,
    SendGameServer = 0x50320110,
    SendDataServer = 0x50320100,

    // =================================================================================
    // Mundo do Jogo & Movimento
    // =================================================================================
    UnitStatusMove = 0x48470010,
    PlayData = 0x48470013,
    PlayDataChar = 0x48470013, // Alias
    UnitStatusContainer = 0x48470014,
    CharacterData = 0x48470020,
    GetPlayerInfoData = 0x48470021,
    UnitInfoQuick = 0x4847004C,
    WarpGateField = 0x48470040,
    DeleteFlagPoint = 0x48470061,
    ClearUnitDataFromMap = 0x6FFF0023,
    MapIndicators = 0x6FFF0026,
    MapSpawnMarkers = 0x6FFF000A,
    GameTimeSync = 0x6FFF0013,

    // =================================================================================
    // Combate & Status
    // =================================================================================
    AttackData = 0x48470030,
    AttackData2 = 0x48470040,
    SingleTargetSkillData = 0x50322000,
    MultiTargetSkillData = 0x50322010,
    AttackResult = 0x50322020,
    LastAttackDamage = 0x50322030,
    DamageInfoContainer = 0x6FFF0017,
    CharacterCombatData = 0x6A6A0030,
    DamageQuick = 0x6A6A0012,
    DamageQuickPVP = 0x6A6A002E,
    QuickHealthUpdate = 0x6FFF0022,
    KillCharacter = 0x50320E00,
    TheDeath = 0x48478800,
    RegenChar = 0x50320B00,
    GameStatus = 0x48470018,
    PvPKill = 0x43550018,
    PVPDataUpdate = 0x4355000F,
    AttackRatingSend = 0x43550003,

    // =================================================================================
    // Skills
    // =================================================================================
    UseSkillNew = 0x435A000F,
    SkillCast = 0x50320A20,
    SkillCastData = 0x50320A30,
    CancelSkill = 0x50320A10,
    SkillStatus = 0x435A000E,
    SkillBuffStatus = 0x6FFF000D,
    BuffSkillsParty = 0x50320A00,
    UseSkillPartyBuffNew = 0x6FFF0020,
    ServerSkillData = 0x6FFF000B,
    ServerSkillInfo = 0x6FFF0016,
    ServerSkillChainData = 0x6FFF0019,
    ServerCommonSkillInfo = 0x6FFF0021,
    Warning_Skill = 0x50322080,
    Resistance = 0x50320a60,
    EvasionSuccess = 0x50320A50,
    Skill_Healing = 0x484700D2,
    Skill_HolyMind = 0x484700D3,

    // =================================================================================
    // Itens, Inventário & Comércio
    // =================================================================================
    PlayItem = 0x48470050,
    NewItem = 0x48470052,
    NewItemAuto = 0x6FFF0025,
    DeleteItemFromMap = 0x48470051,
    ThrowItem = 0x48470053,
    GetItem = 0x4847005A,
    GoldPickup = 0x6A6A002F,
    OpenMyPlayerShop = 0x48478A90,
    OpenPlayerShop = 0x48478AA0,
    PlayerShopConfig = 0x48478AB0,
    PlayerShopTrade = 0x48478AD0,
    PersonalShopMessage = 0x48478AC0,
    TradeData = 0x48470041,
    TradeRequest = 0x48470042,
    TradeItems = 0x48470043,
    TradeReady = 0x48470044,
    OpenCoinShop = 0x48476A08,
    CoinShopBuy = 0x48476A07,
    RecvWarehouse = 0x48470048,
    SendWarehouse = 0x48470047,
    ItemSlotHandle = 0x6A6A001C,
    CheckItemUseSlot = 0x48478930,
    CheckItemUseSlotEx = 0x48478931,
    SetItemTimer = 0x43550008,
    CancelItemTimer = 0x4355001E,
    IntegrityUpdateItem = 0x6A6A0029,

    // =================================================================================
    // Crafting (Mix, Aging, etc.)
    // =================================================================================
    OpenMixItem = 0x48470049,
    MixItem = 0x50320200,
    MixDescription = 0x5A320001,
    OpenAgingMaster = 0x4847004D,
    AgingItem = 0x50320202,
    UpgradeAgingItem = 0x50320204,
    OpenSmelting = 0x50325000,
    OpenResetItem = 0x484700A6,
    ResetItem = 0x50326006,
    OpenManufacture = 0x50326000,
    ManufactureItem = 0x50326001,
    SocketItemDataToServer = 0x48470025,
    SocketItemDataToClient = 0x48470026,
    PerfectItemData = 0x5A320002,
    UseUnionCore = 0x50320206,
    WingItem = 0x50320208,
    MakeLinkCore = 0x50320205,

    // =================================================================================
    // NPCs & Quests
    // =================================================================================
    OpenNPC = 0x48470024,
    NpcData = 0x48470070,
    NpcUnitClick = 0x50320031,
    Shop_ItemList = 0x48470054,
    BuyItemNPCShop = 0x50320210,
    SellItemNPCShop = 0x50320212,
    RepairItemNPCShop = 0x50320213,
    NewItemNPC = 0x6FFF0018,
    QuestCommand = 0x50320220,
    QuestNPCOpen = 0x6A6A0013,
    QuestNPCAccept = 0x6A6A0014,
    QuestNPCFinish = 0x6A6A0018,
    QuestNPCItemList = 0x6A6A0019,
    NpcQuestStatus = 0x6FFF0024,
    QuestDataRequest = 0x6A6A0031,
    QuestDataResponse = 0x6A6A0032,
    QuestNamesRequest = 0x6A6A0033,
    QuestNamesResponse = 0x6A6A0034,
    UpdateQuestData = 0x4355000B,
    FinishQuest = 0x4355000C,
    RestartQuest = 0x4355000D,
    CancelQuest = 0x43550011,
    QuestHandleFinished = 0x6A6A001A,
    RemoveFinishedQuest = 0x6A6A0039,
    MonsterKill = 0x6FFF0030,

    // =================================================================================
    // Social (Party, Chat, etc.)
    // =================================================================================
    ChatGame = 0x48471001,
    ChatMessage = 0x48471005,
    ChatItemLink = 0x48471007,
    RequestParty = 0x6A6A001E,
    JoinParty = 0x6A6A001F,
    UpdateParty = 0x6A6A0020,
    UpdatePartyData = 0x6A6A0021,
    ActionParty = 0x6A6A0022,
    PartyUsePot = 0x484700D0,
    RequestRaid = 0x6A6A0023,
    JoinRaid = 0x6A6A0024,
    UpdateLevelClan = 0x48470038,

    // =================================================================================
    // Efeitos Visuais
    // =================================================================================
    Effect_Item = 0x48478000,
    Effect_Weapon = 0x48478002,
    Effect_Stun = 0x48478004,
    Effect_Curse = 0x48478006,
    Effect_Ice = 0x48478008,
    Effect_Poison = 0x4847800A,
    Effect_Vanish = 0x4847800C,
    Effect_PersonalShop = 0x4847800E,
    Effect_LowLevelPet = 0x4847800F,
    Effect_ClanManager = 0x48479000,
    Effect_ForceOrb = 0x48479010,
    Effect_BlessScore = 0x48479012,
    Effect_User = 0x48479014,
    Effect_Debuff = 0x48479015,
    Effect_KeepSkill = 0x48479020,
    YahooMotion = 0x50320E10,

    // =================================================================================
    // Anti-Cheat & Proteção
    // =================================================================================
    Crash = 0x4847FFFE,
    CrashData = 0x4847FFFD,
    LogCheat = 0x6F6A0001,
    DCAccount = 0x6A6A0001,
    CheckSum = 0x6A6A0006,
    LogDebugger = 0x6A6A0008,
    UndentifiedPacket = 0x6A6A0007,
    WindowList = 0x6A6A000E,
    SettingsData = 0x6A6A000F,
    Client_Error = 0x48478900,
    SetBlacklist = 0x48470033,
    SetBlacklist2 = 0x48478400,
    SetBlacklist3 = 0x48478500,
    Hacktrap = 0x50320a90,
    UseItemCode = 0x4847005b,
    ProcessTimeMax = 0x50320500,
    DamageEncodeMem = 0x50322F00,
    DamageEncodeMem2 = 0x50322EC0,
    ProcessInfo = 0x484700E4,
};

/**
 * @brief Converte um valor de opcode para seu nome em string.
 */
inline std::string getOpcodeName(uint32_t opcode) {
    // Um mapa estático é inicializado apenas uma vez, de forma eficiente.
    static const std::map<uint32_t, std::string> opcodeMap = {
        { static_cast<uint32_t>(Opcodes::KeepAlive), "KeepAlive" },
        { static_cast<uint32_t>(Opcodes::LoginUser), "LoginUser" },
        { static_cast<uint32_t>(Opcodes::AccountLoginCode), "AccountLoginCode" },
        { static_cast<uint32_t>(Opcodes::Ping), "Ping" },
        // Adicione outros mapeamentos aqui
    };

    auto it = opcodeMap.find(opcode);
    if (it != opcodeMap.end()) {
        return it->second;
    }
    return "Opcode Desconhecido";
}

/**
 * @brief Formata os primeiros bytes de um buffer para uma string hexadecimal.
 */
inline std::string formatHex(const std::vector<uint8_t>& data, int count = 16) {
    if (data.empty()) {
        return "[Vazio]";
    }

    std::stringstream hex_stream;
    hex_stream << std::hex << std::setfill('0');

    // Com NOMINMAX definido, esta linha não será mais corrompida pela macro.
    int bytes_to_show = std::min(static_cast<int>(data.size()), count);

    for (int i = 0; i < bytes_to_show; ++i) {
        hex_stream << std::setw(2) << static_cast<int>(data[i]) << " ";
    }

    if (data.size() > static_cast<size_t>(count)) {
        hex_stream << "...";
    }
    return hex_stream.str();
}