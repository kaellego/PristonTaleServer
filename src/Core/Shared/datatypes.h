    #pragma once

#include <cstdint>
#include <windows.h>
#include "Network/Packet.h"

#pragma pack(push, 1)

//Net Server
constexpr uint32_t PKTHDR_NetIdentifier = 0x7F000001;
constexpr uint32_t PKTHDR_NetExp = 0x7F000002;
constexpr uint32_t PKTHDR_NetGetExp = 0x7F000003;
constexpr uint32_t PKTHDR_NetGameMaster = 0x7F000004;
constexpr uint32_t PKTHDR_NetGetGameMaster = 0x7F000005;
constexpr uint32_t PKTHDR_NetClan = 0x7F000006;
constexpr uint32_t PKTHDR_NetDisconnectUser = 0x7F000007;
constexpr uint32_t PKTHDR_NetForceOrb = 0x7F000008;
constexpr uint32_t PKTHDR_NetVersion = 0x7F000009;
constexpr uint32_t PKTHDR_NetCharacterSync = 0x7F00000A;
constexpr uint32_t PKTHDR_NetGiveExp = 0x7F00000B;
constexpr uint32_t PKTHDR_NetPingPong = 0x7F00000C;
constexpr uint32_t PKTHDR_NetBCStatusReq = 0x7F00000D;
constexpr uint32_t PKTHDR_NetBCStatusSnd = 0x7F00000E;
constexpr uint32_t PKTHDR_NetBCNewOwnerClanID = 0x7F00000F;
constexpr uint32_t PKTHDR_NetBotAdd = 0x7F000010;
constexpr uint32_t PKTHDR_NetPremiumEXP = 0x7F000011;
constexpr uint32_t PKTHDR_NetUnloadUser = 0x7F000012;
constexpr uint32_t PKTHDR_NetEXPEvent = 0x7F000013;
constexpr uint32_t PKTHDR_NetTradeCoin = 0x7F000014;
constexpr uint32_t PKTHDR_NetUsersOnline = 0x7F000015;
constexpr uint32_t PKTHDR_NetQuestT5Data = 0x7F000016;
constexpr uint32_t PKTHDR_NetQuestUpdateData = 0x7F000017;
constexpr uint32_t PKTHDR_NetQuestUpdateDataPart = 0x7F000018;
constexpr uint32_t PKTHDR_NetQuestCancelOrFinish = 0x7F000019;
constexpr uint32_t PKTHDR_NetLoadPVPData = 0x7F000020;
constexpr uint32_t PKTHDR_NetLoadQuestData = 0x7F000021;
constexpr uint32_t PKTHDR_NetSyncFinishedQuestData = 0x7F000022; //to game servers
constexpr uint32_t PKTHDR_NetPlayerWorldToken = 0x7F000040;
constexpr uint32_t PKTHDR_NetPlayerInventory = 0x7F000041;
constexpr uint32_t PKTHDR_NetPlayerGold = 0x7F000042;
constexpr uint32_t PKTHDR_NetPlayerGoldDiff = 0x7F000043;
constexpr uint32_t PKTHDR_NetPlayerThrow = 0x7F000044;
constexpr uint32_t PKTHDR_NetStartMaintenance = 0x7F000045;
constexpr uint32_t PKTHDR_NetPlayerItemPut = 0x7F000046;
constexpr uint32_t PKTHDR_NetEasterEvent = 0x7F000047;
constexpr uint32_t PKTHDR_NetAgingEvent = 0x7F000048;
constexpr uint32_t PKTHDR_NetBellatraSoloRanking = 0x7F000049;
constexpr uint32_t PKTHDR_NetHallowenEvent = 0x7F000050;
constexpr uint32_t PKTHDR_NetOpenWarehouse = 0x7F000051;
constexpr uint32_t PKTHDR_NetOpenWarehouseSeasonal = 0x7F000052;
constexpr uint32_t PKTHDR_NetChristmasEvent = 0x7F000053;
constexpr uint32_t PKTHDR_NetFreeEventGirl = 0x7F000054;
constexpr uint32_t PKTHDR_NetStopMaintenance = 0x7F000056;
constexpr uint32_t PKTHDR_NetLevelUp = 0x7F000058;
constexpr uint32_t PKTHDR_NetSetVersion = 0x7F000059;
constexpr uint32_t PKTHDR_NetReloadCoinShop = 0x7F000060;
constexpr uint32_t PKTHDR_NetClearTickRO = 0x7F000061;
constexpr uint32_t PKTHDR_NetClearTickChristmas = 0x7F000062;
constexpr uint32_t PKTHDR_NetKickAll = 0x7F000063;
constexpr uint32_t PKTHDR_NetMuteSync = 0x7F000064;
constexpr uint32_t PKTHDR_NetGetMuteSync = 0x7F000065;
constexpr uint32_t PKTHDR_NetStarWarsEvent = 0x7F000066;
constexpr uint32_t PKTHDR_NetBeeEvent = 0x7F000067;
constexpr uint32_t PKTHDR_NetOpenItemDistributor = 0x7F000068;
constexpr uint32_t PKTHDR_NetGameMasterCommand = 0x7F000069;
constexpr uint32_t PKTHDR_NetPlayDataEx = 0x7F000070; //game server to login server
constexpr uint32_t PKTHDR_NetRemoveFinishedQuestData = 0x7F000071; //game server to login server or vice versa
constexpr uint32_t PKTHDR_NetSendItemData = 0x7F000072; //game server to login server
constexpr uint32_t PKTHDR_NetMimicEvent = 0x7F000073; //game server to login server

constexpr uint32_t PKTHDR_Connected = 0x48470080;

//Packets Item Data
constexpr uint32_t PKTHDR_PerfectItemData = 0x5A320002;
constexpr uint32_t PKTHDR_MixDescription = 0x5A320001;
constexpr uint32_t PKTHDR_OpenMixItem = 0x48470049;
constexpr uint32_t PKTHDR_RecvWarehouse = 0x48470048;
constexpr uint32_t PKTHDR_SendWarehouse = 0x48470047;
constexpr uint32_t PKTHDR_TradeData = 0x48470041;
constexpr uint32_t PKTHDR_TradeRequest = 0x48470042;
constexpr uint32_t PKTHDR_TradeItems = 0x48470043; //Trade Time...( when trade your inventory item to player... )
constexpr uint32_t PKTHDR_TradeReady = 0x48470044;
constexpr uint32_t PKTHDR_ThrowItem = 0x48470053; //smTRANSCODE_THROWITEM
constexpr uint32_t PKTHDR_CheckItemUseSlot = 0x48478930;
constexpr uint32_t PKTHDR_CheckItemUseSlotEx = 0x48478931;

constexpr uint32_t PKTHDR_Skill_Healing = 0x484700D2; //smTRANSCODE_HEALING
constexpr uint32_t PKTHDR_Skill_HolyMind = 0x484700D3; //smTRANSCODE_HOLYMIND
constexpr uint32_t PKTHDR_Skill_GrandHealing = 0x484700D4; //smTRANSCODE_GRAND_HEALING


constexpr uint32_t PKTHDR_KeySet = 0x4847FFFF;
constexpr uint32_t PKTHDR_Crash = 0x4847FFFE;
constexpr uint32_t PKTHDR_CrashData = 0x4847FFFD;

// Packets Character Data
constexpr uint32_t PKTHDR_PacketFun = 0x43550001;
constexpr uint32_t PKTHDR_SetExp = 0x43550002;
constexpr uint32_t PKTHDR_AttackRatingSend = 0x43550003;
constexpr uint32_t PKTHDR_DamageDebugInfo = 0x43550004;
constexpr uint32_t PKTHDR_ServerInfo = 0x43550005;
constexpr uint32_t PKTHDR_PlayDataEx = 0x43550006;	//received from each player every 5 seconds
constexpr uint32_t PKTHDR_PremiumData = 0x43550007;
constexpr uint32_t PKTHDR_SetItemTimer = 0x43550008;
constexpr uint32_t PKTHDR_DisableGM = 0x43550009;
constexpr uint32_t PKTHDR_QuestData = 0x4355000A; ///< Obselete
constexpr uint32_t PKTHDR_UpdateQuestData = 0x4355000B;
constexpr uint32_t PKTHDR_FinishQuest = 0x4355000C;
constexpr uint32_t PKTHDR_RestartQuest = 0x4355000D;
constexpr uint32_t PKTHDR_BellatraCrown = 0x4355000E;
constexpr uint32_t PKTHDR_PVPDataUpdate = 0x4355000F;
constexpr uint32_t PKTHDR_UpdateSWPhase = 0x43550010;
constexpr uint32_t PKTHDR_CancelQuest = 0x43550011;
constexpr uint32_t PKTHDR_CrystalUse = 0x43550012;
constexpr uint32_t PKTHDR_CrystalUseDie = 0x4355001;
constexpr uint32_t PKTHDR_UserOnline = 0x43550014;
constexpr uint32_t PKTHDR_CancelForceOrb = 0x43550015;
constexpr uint32_t PKTHDR_FireMeteorite = 0x43550016;
constexpr uint32_t PKTHDR_NPCEasterEgg = 0x43550017;
constexpr uint32_t PKTHDR_PvPKill = 0x43550018;
constexpr uint32_t PKTHDR_NPCIhinRewardRequest = 0x43550019;
constexpr uint32_t PKTHDR_CharacterHeadOldUse = 0x4355001A;
constexpr uint32_t PKTHDR_BlessCastleIndividualScore = 0x4355001B;
constexpr uint32_t PKTHDR_BlessCastleUserScoreUpdate = 0x4355001C;
constexpr uint32_t PKTHDR_NPCRudolphChristmas = 0x4355001D;
constexpr uint32_t PKTHDR_CancelItemTimer = 0x4355001E;
constexpr uint32_t PKTHDR_NPCMixGuild = 0x4355001F; //xxstr mix guild
constexpr uint32_t PKTHDR_NPCRankList = 0x43550020; //xxstr rank list
constexpr uint32_t PKTHDR_TOP_LEVEL = 0x44550022; //xxstr top level
constexpr uint32_t PKTHDR_WrapCamera = 0x44550024;
constexpr uint32_t PKTHDR_BlessCastleTopClanSync = 0x44550025;

//jlm - seperated packets
constexpr uint32_t PKTHDR_NPCRankListAllPlayers = 0x43550030;
constexpr uint32_t PKTHDR_NPCRankListPVP = 0x43550031;
constexpr uint32_t PKTHDR_NPCRankListByClass = 0x43550032;
constexpr uint32_t PKTHDR_NPCRankListClanSod = 0x43550033;
constexpr uint32_t PKTHDR_NPCRankListIndividualSod = 0x43550034;
constexpr uint32_t PKTHDR_NPCRankListBlessedCastle = 0x43550035;
constexpr uint32_t PKTHDR_NPCRankListSoloSod = 0x43550037;

//Packets Protect
constexpr uint32_t PKTHDR_LogCheat = 0x6F6A0001;
constexpr uint32_t PKTHDR_DCAccount = 0x6A6A0001;
constexpr uint32_t PKTHDR_ReconnectedServer = 0x6A6A0002;
constexpr uint32_t PKTHDR_SendToLoginServer = 0x6A6A0003;
constexpr uint32_t PKTHDR_Microphone = 0x6A6A0004;
constexpr uint32_t PKTHDR_LogDataSimple = 0x6A6A0005;
constexpr uint32_t PKTHDR_CheckSum = 0x6A6A0006;
constexpr uint32_t PKTHDR_LogDebugger = 0x6A6A0008;
constexpr uint32_t PKTHDR_UndentifiedPacket = 0x6A6A0007;
constexpr uint32_t PKTHDR_AddExp = 0x6A6A0009;
constexpr uint32_t PKTHDR_LoseExp = 0x6A6A000A;
constexpr uint32_t PKTHDR_GetItemData = 0x6A6A000B;
constexpr uint32_t PKTHDR_SetItemData = 0x6A6A000C;
constexpr uint32_t PKTHDR_SetGold = 0x6A6A000D;
constexpr uint32_t PKTHDR_WindowList = 0x6A6A000E;
constexpr uint32_t PKTHDR_SettingsData = 0x6A6A000F;
constexpr uint32_t PKTHDR_RetryWarehouse = 0x6A6A0010;
constexpr uint32_t PKTHDR_WarehouseSaveSuccess = 0x6A6A0011;
constexpr uint32_t PKTHDR_DamageQuick = 0x6A6A0012;
constexpr uint32_t PKTHDR_QuestNPCOpen = 0x6A6A0013;
constexpr uint32_t PKTHDR_QuestNPCAccept = 0x6A6A0014;
constexpr uint32_t PKTHDR_QuestStartData = 0x6A6A0015;
constexpr uint32_t PKTHDR_QuestUpdateData = 0x6A6A0016;
constexpr uint32_t PKTHDR_QuestCancel = 0x6A6A0017;
constexpr uint32_t PKTHDR_QuestNPCFinish = 0x6A6A0018;
constexpr uint32_t PKTHDR_QuestNPCItemList = 0x6A6A0019;
constexpr uint32_t PKTHDR_QuestHandleFinished = 0x6A6A001A;
constexpr uint32_t PKTHDR_CharacterDataEx = 0x6A6A001B;
constexpr uint32_t PKTHDR_ItemSlotHandle = 0x6A6A001C;
constexpr uint32_t PKTHDR_T5QuestArena = 0x6A6A001D;
constexpr uint32_t PKTHDR_RequestParty = 0x6A6A001E;
constexpr uint32_t PKTHDR_JoinParty = 0x6A6A001F;
constexpr uint32_t PKTHDR_UpdateParty = 0x6A6A0020;
constexpr uint32_t PKTHDR_UpdatePartyData = 0x6A6A0021;
constexpr uint32_t PKTHDR_ActionParty = 0x6A6A0022;
constexpr uint32_t PKTHDR_RequestRaid = 0x6A6A0023;
constexpr uint32_t PKTHDR_JoinRaid = 0x6A6A0024;
constexpr uint32_t PKTHDR_RollDiceDropItem = 0x6A6A0025;
constexpr uint32_t PKTHDR_RollDiceRanking = 0x6A6A0026;
constexpr uint32_t PKTHDR_QuestItemSwap = 0x6A6A0027;
constexpr uint32_t PKTHDR_BossTimeUpdate = 0x6A6A0028;
constexpr uint32_t PKTHDR_IntegrityUpdateItem = 0x6A6A0029;
constexpr uint32_t PKTHDR_UpdatePetDataEx = 0x6A6A002A;
constexpr uint32_t PKTHDR_DynamicQuestOpen = 0x6A6A002B;
constexpr uint32_t PKTHDR_DynamicQuestOpenNPC = 0x6A6A002C;
constexpr uint32_t PKTHDR_QuestHandleFinishedContainer = 0x6A6A002D;
constexpr uint32_t PKTHDR_DamageQuickPVP = 0x6A6A002E;
constexpr uint32_t PKTHDR_GoldPickup = 0x6A6A002F;
constexpr uint32_t PKTHDR_CharacterCombatData = 0x6A6A0030;	//mainly for pvp
constexpr uint32_t PKTHDR_QuestDataRequest = 0x6A6A0031;
constexpr uint32_t PKTHDR_QuestDataResponse = 0x6A6A0032;
constexpr uint32_t PKTHDR_QuestNamesRequest = 0x6A6A0033;
constexpr uint32_t PKTHDR_QuestNamesResponse = 0x6A6A0034;
constexpr uint32_t PKTHDR_NewQuestNamesPostLvlUp = 0x6A6A0035; //addition quests post level up
constexpr uint32_t PKTHDR_NeverSinkTeleportRequest = 0x6A6A0036;
constexpr uint32_t PKTHDR_NeverSinkOpenUI = 0x6A6A0037;
constexpr uint32_t PKTHDR_MissingRequiredQuestItems = 0x6A6A0038; //items didn't exist on client side on completion
constexpr uint32_t PKTHDR_RemoveFinishedQuest = 0x6A6A0039;
constexpr uint32_t PKTHDR_RollDiceDropUserResult = 0x6A6A0040;


//Coin Shop
constexpr uint32_t PKTHDR_CoinShopNPC = 0x48476A01;
constexpr uint32_t PKTHDR_CoinShopAmount = 0x48476A02;
constexpr uint32_t PKTHDR_CoinShopOpenTab = 0x48476A03;
constexpr uint32_t PKTHDR_CoinShopTab = 0x48476A04;
constexpr uint32_t PKTHDR_CoinShopOpenItemData = 0x48476A05;
constexpr uint32_t PKTHDR_CoinShopItemData = 0x48476A06;
constexpr uint32_t PKTHDR_CoinShopBuy = 0x48476A07;
constexpr uint32_t PKTHDR_OpenCoinShop = 0x48476A08;
constexpr uint32_t PKTHDR_CoinServiceOpen = 0x48476A09;
constexpr uint32_t PKTHDR_CoinServiceBuy = 0x48476A0A;
constexpr uint32_t PKTHDR_CoinServiceAgingRecoveryOpen = 0x48476A0B;
constexpr uint32_t PKTHDR_CoinTraded = 0x48476A0C;
constexpr uint32_t PKTHDR_CoinTradeSet = 0x48476A0D;
constexpr uint32_t PKTHDR_PlayerInfo2 = 0x48470032; //smTRANSCODE_PLAYERINFO2

//Packets Event
constexpr uint32_t PKTHDR_KillsInSod = 0x503220B1;

constexpr uint32_t PKTHDR_CHECKITEM = 0x50320300; //smTRANSCODE_CHECKITEM
constexpr uint32_t PKTHDR_LIMIT_DAMAGE = 0x50322050; //smTRANSCODE_LIMIT_DAMAGE
constexpr uint32_t PKTHDR_BLESSCASTLE_INFO = 0x503220C0; //smTRANSCODE_BLESSCASTLE_INFO
constexpr uint32_t PKTHDR_BLESSCASTLE_TAX = 0x503220D0; //smTRANSCODE_BLESSCASTLE_TAX

//Packets Sync Data
constexpr uint32_t PKTHDR_ChatGame = 0x48471001;
constexpr uint32_t PKTHDR_ChatMessage = 0x48471005;   //smTRANSCODE_WHISPERMESSAGE (pops up a chat message above the char)
constexpr uint32_t PKTHDR_ChatItemLink = 0x48471007;
constexpr uint32_t PKTHDR_PersonalShopMessage = 0x48478AC0;
constexpr uint32_t PKTHDR_SyncData1 = 0x4847008B;
constexpr uint32_t PKTHDR_SyncData2 = 0x4847008E;
constexpr uint32_t PKTHDR_CharacterData = 0x48470020;   //smTRANSCODE_PLAYERINFO    (can also be for units)
constexpr uint32_t PKTHDR_GetPlayerInfoData = 0x48470021;   //smTRANSCODE_GETPLAYERINFO (can also be for units)
constexpr uint32_t PKTHDR_Ping = 0x435A0007;
constexpr uint32_t PKTHDR_PingLoginServer = 0x435A0008;
constexpr uint32_t PKTHDR_FuryArena = 0x435A0009;
constexpr uint32_t PKTHDR_FuryArenaItemData = 0x435A000A;
constexpr uint32_t PKTHDR_FuryArenaBossData = 0x435A000B;
constexpr uint32_t PKTHDR_NPCLarryOpen = 0x435A000C;
constexpr uint32_t PKTHDR_SkillStatus = 0x435A000E;
constexpr uint32_t PKTHDR_UseSkillNew = 0x435A000F;
constexpr uint32_t PKTHDR_NPCMarinaOpen = 0x435A0010;
constexpr uint32_t PKTHDR_SaveAndClose = 0x435A0011;
constexpr uint32_t PKTHDR_PhoenixPet = 0x484700B0;
constexpr uint32_t PKTHDR_Warehouse = 0x48470047;
constexpr uint32_t PKTHDR_Save = 0x484700E8;
constexpr uint32_t PKTHDR_Disconnect = 0x484700E6;
constexpr uint32_t PKTHDR_Reconnect = 0x48478010;
constexpr uint32_t PKTHDR_ReconnectLogin = 0x48478011;
constexpr uint32_t PKTHDR_UpdateLevelClan = 0x48470038;
constexpr uint32_t PKTHDR_UnitStatusContainer = 0x48470014;	//smTRANSCODE_PLAYDATAGROUP - data about other player's unit
constexpr uint32_t PKTHDR_GameStatus = 0x48470018;   //smTRANSCODE_PLAY_COMMAND
constexpr uint32_t PKTHDR_KillCharacter = 0x50320E00;
constexpr uint32_t PKTHDR_RegenChar = 0x50320B00;	//smTRANSCODE_VIRTURAL_POTION (heals char)
constexpr uint32_t PKTHDR_UpdatePetData = 0x50320A40;	//smTRANSCODE_UPDATEL_SKILL (but only used for pets)
constexpr uint32_t PKTHDR_AttackData = 0x48470030;	//smTRANSCODE_ATTACKDATA
constexpr uint32_t PKTHDR_AttackData2 = 0x48470040;	//smTRANSCODE_ATTACKDATA
constexpr uint32_t PKTHDR_GoldBellatraClan = 0x48478A18;
constexpr uint32_t PKTHDR_OpenMyPlayerShop = 0x48478A90;
constexpr uint32_t PKTHDR_OpenPlayerShop = 0x48478AA0;
constexpr uint32_t PKTHDR_PlayerShopConfig = 0x48478AB0;
constexpr uint32_t PKTHDR_PlayerShopTrade = 0x48478AD0;
constexpr uint32_t PKTHDR_LastAttackDamage = 0x50322030;   //smTRANSCODE_DISP_DAMAGE
constexpr uint32_t PKTHDR_SingleTargetSkillData = 0x50322000;	//smTRANSCODE_ATTACK_DAMAGE (damage) - was PKTHDR_AttackDamage
constexpr uint32_t PKTHDR_MultiTargetSkillData = 0x50322010;	//smTRANSCODE_RANGE_DAMAGE  (damage by ranged skill) - was PKTHDR_RangeDamage
constexpr uint32_t PKTHDR_AttackResult = 0x50322020;	//smTRANSCODE_ATTACK_RESULT (used for crit aging..)
constexpr uint32_t PKTHDR_BuyItemNPCShop = 0x50320210;	//smTRANSCODE_SHOPTITEM
constexpr uint32_t PKTHDR_SellItemNPCShop = 0x50320212;
constexpr uint32_t PKTHDR_RepairItemNPCShop = 0x50320213;
constexpr uint32_t PKTHDR_ForceOrbData = 0x50320030;
constexpr uint32_t PKTHDR_NpcUnitClick = 0x50320031;
constexpr uint32_t PKTHDR_QuestCommand = 0x50320220; //smTRANSCODE_QUEST_COMMAND

// Packets Login
constexpr uint32_t PKTHDR_ServerList = 0x484700C0;
constexpr uint32_t PKTHDR_SaveData = 0x48470081;  //smTRANSCODE_RECORDDATA
constexpr uint32_t PKTHDR_SelectCharacter = 0x48470082;
constexpr uint32_t PKTHDR_FailRecordData = 0x48470083;  //smTRANSCODE_FAILRECORDDATA
constexpr uint32_t PKTHDR_UserInfo = 0x48470086;
constexpr uint32_t PKTHDR_CreateCharacter = 0x48470088;
constexpr uint32_t PKTHDR_DeleteCharacter = 0x48470087;
constexpr uint32_t PKTHDR_Version = 0x4847008A;
constexpr uint32_t PKTHDR_AccountLoginCode = 0x48470023;

constexpr uint32_t PKTHDR_Shop_ItemList = 0x48470054; // smTRANSCODE_SHOP_ITEMLIST

constexpr uint32_t PKTHDR_LoginUser = 0x48480001;

//Packet Skills
constexpr uint32_t PKTHDR_BuffSkillsParty = 0x50320A00;
constexpr uint32_t PKTHDR_CancelDrasticSpirit = 0x50320A11;
constexpr uint32_t PKTHDR_CancelSkill = 0x50320A10;	//smTRANSCODE_CANCEL_SKILL
constexpr uint32_t PKTHDR_SkillCast = 0x50320A20;	//smTRANSCODE_PROCESS_SKILL
constexpr uint32_t PKTHDR_SkillCastData = 0x50320A30;	//smTRANSCODE_PROCESS_SKILL2 0x50320A30
constexpr uint32_t PKTHDR_EvasionSuccess = 0x50320A50;
constexpr uint32_t PKTHDR_PartyUsePot = 0x484700D0;
constexpr uint32_t PKTHDR_HolyMind = 0x484700D3;
constexpr uint32_t PKTHDR_Wisp = 0x484700C3;


//Packets Unit
constexpr uint32_t PKTHDR_OpenNPC = 0x48470024;
constexpr uint32_t PKTHDR_NpcData = 0x48470070;
constexpr uint32_t PKTHDR_SetExpOld_DontUse = 0x48470031;	//smTRANSCODE_ADDEXP (don't use; it actually sets exp in Asm)
constexpr uint32_t PKTHDR_TeleportEvent = 0x4847005D;
constexpr uint32_t PKTHDR_UnitStatusMove = 0x48470010;	//smTRANSCODE_PLAYDATA1
constexpr uint32_t PKTHDR_PlayData = 0x48470013;	//smTRANSCODE_TRANSPLAYDATA
constexpr uint32_t PKTHDR_UnitInfoQuick = 0x4847004C;	//smTRANSCODE_PLAYERINFO_Q
constexpr uint32_t PKTHDR_OpenEventGirl = 0x4847004F;	//smTRANSCODE_OPEN_EVENTGIFT
constexpr uint32_t PKTHDR_OpenAgingMaster = 0x4847004D;	//smTRANSCODE_OPEN_AGING
constexpr uint32_t PKTHDR_OpenStarPoint = 0x48478A10;   //smTRANSCODE_OPEN_STARPOINT
constexpr uint32_t PKTHDR_OpenCollect = 0x4847004A;   //smTRANSCODE_OPEN_COLLECT
constexpr uint32_t PKTHDR_OpenGiveMoney = 0x48478A14;   //smTRANSCODE_OPEN_GIVEMONEY
constexpr uint32_t PKTHDR_OpenClanMenu = 0x48478A00;   //smTRANSCODE_OPEN_CLANMENU
constexpr uint32_t PKTHDR_PublicPolling = 0x50320A70;   //smTRANSCODE_PUBLIC_POLLING

//Effect

constexpr uint32_t PTKHDR_Effect_Item = 0x48478000;	//smTRANSCODE_EFFECT_ITEM
constexpr uint32_t PTKHDR_Effect_Weapon = 0x48478002;	//smTRANSCODE_EFFECT_WEAPON	- calls SetTransEffectWeapon on client side (2012)
constexpr uint32_t PTKHDR_Effect_Stun = 0x48478004;	//smTRANSCODE_EFFECT_STUN
constexpr uint32_t PTKHDR_Effect_Curse = 0x48478006;	//smTRANSCODE_EFFECT_CURSE
constexpr uint32_t PTKHDR_Effect_Ice = 0x48478008;	//smTRANSCODE_EFFECT_ICE
constexpr uint32_t PTKHDR_Effect_Poison = 0x4847800A;	//smTRANSCODE_EFFECT_POISION
constexpr uint32_t PTKHDR_Effect_Vanish = 0x4847800C;	//smTRANSCODE_EFFECT_VANISH
constexpr uint32_t PTKHDR_Effect_PersonalShop = 0x4847800E;	//smTRANSCODE_EFFECT_PERSHOP
constexpr uint32_t PTKHDR_Effect_LowLevelPet = 0x4847800F;	//smTRANSCODE_EFFECT_LOWPET
constexpr uint32_t PTKHDR_Effect_ClanManager = 0x48479000;	//smTRANSCODE_EFFECT_CLANMGR
constexpr uint32_t PTKHDR_Effect_ForceOrb = 0x48479010;	//smTRANSCODE_EFFECT_FORCEORB
constexpr uint32_t PTKHDR_Effect_BlessScore = 0x48479012;	//smTRANSCODE_EFFECT_BLESS_SCORE
constexpr uint32_t PKTHDR_Effect_User = 0x48479014;	//new
constexpr uint32_t PTKHDR_Effect_Debuff = 0x48479015;	//new
constexpr uint32_t PTKHDR_Effect_KeepSkill = 0x48479020;	//smTRANSCODE_EFFECT_KEEPSKILL

//Packets Map
constexpr uint32_t PKTHDR_WarpGateField = 0x48470040;   //smTRANSCODE_WARPFIELD
constexpr uint32_t PKTHDR_DeleteFlagPoint = 0x48470061;
constexpr uint32_t PKTHDR_TheDeath = 0x48478800;
constexpr uint32_t PKTHDR_PlayItem = 0x48470050;   //show item on floor - smTRANSCODE_PLAYITEM
constexpr uint32_t PKTHDR_SaveThrowMoney = 0x48470056;	//received via game server
constexpr uint32_t PKTHDR_SaveThrowItem2 = 0x48470057;	//gameserver to login server

constexpr uint32_t PKTHDR_UseUnionCore = 0x50320206;
constexpr uint32_t PKTHDR_WingItem = 0x50320208;
constexpr uint32_t PKTHDR_BellatraInformation = 0x503220B0;

//Packets Item
constexpr uint32_t PKTHDR_SocketItemDataToServer = 0x48470025;
constexpr uint32_t PKTHDR_SocketItemDataToClient = 0x48470026;
constexpr uint32_t PKTHDR_DeleteItemFromMap = 0x48470051;   //Removes item from map
constexpr uint32_t PKTHDR_NewItem = 0x48470052;   //smTRANSCODE_PUTITEM
constexpr uint32_t PKTHDR_GetItem = 0x4847005A;
constexpr uint32_t PKTHDR_PaymentMoney = 0x4847005E;
constexpr uint32_t PKTHDR_UpgradeAgingItem = 0x50320204;
constexpr uint32_t PKTHDR_AgingItem = 0x50320202;	//smTRANSCODE_AGINGITEM (also used for maturing)
constexpr uint32_t PKTHDR_OpenSmelting = 0x50325000;
constexpr uint32_t PKTHDR_OpenResetItem = 0x484700A6;
constexpr uint32_t PKTHDR_OpenManufacture = 0x50326000;
constexpr uint32_t PKTHDR_ManufactureItem = 0x50326001;
constexpr uint32_t PKTHDR_MixItem = 0x50320200;
constexpr uint32_t PKTHDR_ResetItem = 0x50326006;
constexpr uint32_t PKTHDR_NPCItemBox = 0x48478A80;


//others
constexpr uint32_t PKTHDR_RecordResult = 0x48470084;
constexpr uint32_t PKTHDR_SendGameServer = 0x50320110;
constexpr uint32_t PKTHDR_DamageEncodeMem = 0x50322F00;
constexpr uint32_t PKTHDR_DamageEncodeMem2 = 0x50322EC0;

constexpr uint32_t PKTHDR_NETWORK_QUALITY = 0x484700E0;

constexpr uint32_t PKTHDR_CONNECTED = 0x48470080;
constexpr uint32_t PKTHDR_DAMAGE_ENCODE_MEM2 = 0x50322EC0;
constexpr uint32_t PKTHDR_UPDATE_SERVER_PARAM = 0x503220F0;
constexpr uint32_t PKTHDR_PROCESSINFO = 0x484700E4;

constexpr uint32_t PKTHDR_SET_BLACKLIST = 0x48470033;	//smTRANSCODE_SET_BLACKLIST - Hacking User Registration 2
constexpr uint32_t PKTHDR_SET_BLACKLIST2 = 0x48478400;	//smTRANSCODE_SET_BLACKLIST2 - Hacking User Registration 2
constexpr uint32_t PKTHDR_SET_BLACKLIST3 = 0x48478500;	//smTRANSCODE_SET_BLACKLIST3 - Hacking User Registration 3
constexpr uint32_t PKTHDR_HACKTRAP = 0x50320a90;	//smTRANSCODE_HACKTRAP
constexpr uint32_t PKTHDR_USEITEM_CODE = 0x4847005b;   //smTRANSCODE_USEITEM_CODE
constexpr uint32_t PKTHDR_MAKE_LINKCORE = 0x50320205;   //smTRANSCODE_MAKE_LINKCORE

constexpr uint32_t PKTHDR_PROCESS_TIMEMAX = 0x50320500;   //smTRANSCODE_PROCESS_TIMEMAX
constexpr uint32_t PKTHDR_DAMAGE_ENCODE_MEM = 0x50322F00;
constexpr uint32_t PKTHDR_SEND_GAMESERVER = 0x50320110;

//NEW
//NOTE - max value should be 0x7FFFFFFF

constexpr uint32_t PTKHDR_EventGirlReset = 0x6FFF0001;
constexpr uint32_t PKTHDR_RankingListRequest = 0x6FFF0002;
constexpr uint32_t PKTHDR_NumfOfInitialBellatraPlayers = 0x6FFF0003;
constexpr uint32_t PKTHDR_ExtraUserData = 0x6FFF0004;
constexpr uint32_t PKTHDR_SoloRankingListRequest = 0x6FFF0005;
constexpr uint32_t PKTHDR_PremiumDataSync = 0x6FFF0006;	//for syncing timers of premium items to client
constexpr uint32_t PKTHDR_MiscUnitCommand = 0x6FFF0007;
constexpr uint32_t PKTHDR_ForceOrbCancel = 0x6FFF0009;
constexpr uint32_t PKTHDR_MapSpawnMarkers = 0x6FFF000A;	//for map spawn markers
constexpr uint32_t PKTHDR_ServerSkillData = 0x6FFF000B;	//server skill data
constexpr uint32_t PKTHDR_SkillBuffStatus = 0x6FFF000D;
constexpr uint32_t PKTHDR_HalloweenEventStatus = 0x6FFF000E;
constexpr uint32_t PKTHDR_TitleList = 0x6FFF000F;
constexpr uint32_t PKTHDR_SetTitle = 0x6FFF0010;
constexpr uint32_t PKTHDR_RecoverItem = 0x6FFF0011;
constexpr uint32_t PKTHDR_EnterBellatraCastle = 0x6FFF0012;
constexpr uint32_t PKTHDR_GameTimeSync = 0x6FFF0013;
constexpr uint32_t PKTHDR_BellatraCrownFull = 0x6FFF0014;
constexpr uint32_t PKTHDR_BellatraCrownSolo = 0x6FFF0015;
constexpr uint32_t PKTHDR_ServerSkillInfo = 0x6FFF0016;
constexpr uint32_t PKTHDR_DamageInfoContainer = 0x6FFF0017;
constexpr uint32_t PKTHDR_NewItemNPC = 0x6FFF0018;
constexpr uint32_t PKTHDR_ServerSkillChainData = 0x6FFF0019;
constexpr uint32_t PKTHDR_UseSkillPartyBuffNew = 0x6FFF0020;
constexpr uint32_t PKTHDR_ServerCommonSkillInfo = 0x6FFF0021;
constexpr uint32_t PKTHDR_QuickHealthUpdate = 0x6FFF0022;
constexpr uint32_t PKTHDR_ClearUnitDataFromMap = 0x6FFF0023;
constexpr uint32_t PKTHDR_NpcQuestStatus = 0x6FFF0024;
constexpr uint32_t PKTHDR_NewItemAuto = 0x6FFF0025;
constexpr uint32_t PKTHDR_MapIndicators = 0x6FFF0026;
constexpr uint32_t PKTHDR_QuestMonsterName = 0x6FFF0027;
constexpr uint32_t PKTHDR_QuestMonsterNameReq = 0x6FFF0028;
constexpr uint32_t PKTHDR_UpgradeQuestWeapon = 0x6FFF0029;
constexpr uint32_t PKTHDR_MonsterKill = 0x6FFF0030;
constexpr uint32_t PKTHDR_SendStablePet = 0x6FFF0031;
constexpr uint32_t PKTHDR_SendStablePet_Change_Name = 0x6FFF0032;
constexpr uint32_t PKTHDR_SendStablePet_ActivePet = 0x6FFF0033;

constexpr uint32_t PKTHDR_Warning_Skill = 0x50322080;   //smTRANSCODE_WARNING_SKILL
constexpr uint32_t PKTHDR_Client_Error = 0x48478900;   //smTRANSCODE_CLIENT_ERROR
constexpr uint32_t PKTHDR_Resistance = 0x50320a60;   //smTRANSCODE_RESISTANCE

constexpr uint32_t PKTHDR_SendDataServer = 0x50320100;   //smTRANSCODE_SEND_DATASERVER - Transfer connection to data server

constexpr uint32_t PKTHDR_Unknown1 = 0x50320040;
constexpr uint32_t PKTHDR_Unknown2 = 0x50326070;
constexpr uint32_t PKTHDR_Unknown3 = 0x50320120;
constexpr uint32_t PKTHDR_Unknown4 = 0x7f000051;  //related to WH
constexpr uint32_t PKTHDR_Unknown5 = 0x48470109;
constexpr uint32_t PKTHDR_Unknown6 = 0x48470045;  //related to skill master
constexpr uint32_t PKTHDR_Unknown7 = 0x50320039;  //related to item deletion when expired

constexpr uint32_t PKTHDR_Unknown8 = 0x50320C00;
constexpr uint32_t PKTHDR_Unknown9 = 0x50326030;
constexpr uint32_t PKTHDR_Unknown10 = 0x50326002;
constexpr uint32_t PKTHDR_Unknown11 = 0x50320800;
constexpr uint32_t PKTHDR_Unknown12 = 0x50326040;

constexpr uint32_t PKTHDR_CommandUser = 0x4847005c;  //smTRANSCODE_COMMAND_USER
constexpr uint32_t PKTHDR_YahooMotion = 0x50320E10;  //smTRANSCODE_YAHOO_MOTION - Execute yay action
//1345454352 -> 0x50320110 ?? --> SEND_GAMESERVER
//1345466112 -> 0x50322F00 ?? --> DAMAGE_ENCODE_MEM
//1345466048 -> 0x50322EC0 ?? --> DAMAGE_ENCODE_MEM2

//Este é o arquivo principal. Ele conterá todas as definições de struct, especialmente as 
//estruturas de pacotes. A herança : Packet foi substituída por um membro PacketHeader
//header;. Isso garante um layout de memória previsível e limpo.

enum class EBanStatus {
    BANSTATUS_NotBanned,
    BANSTATUS_Banned,
    BANSTATUS_TempBanned,
    BANSTATUS_BanExpired
};

// --- Estruturas de Pacotes de Rede ---

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

struct HPStruct {
    int16_t sMin;
    int16_t sMax;
};

struct StructFuryArenaBoss {
    int32_t iCurrentBossHP;
    int32_t iMaxBossHP;
};

struct Point3D {
    int32_t iX;
    int32_t iY;
    int32_t iZ;
};

struct PacketRankingListPVP {
    PacketHeader header;
    char    szPVPCharName[10][32];
    int32_t iPVPKills[10];
    int32_t iPVPDeaths[10];
};

struct PacketRankingListByClass {
    PacketHeader header;
    int32_t iClass;
    int32_t iCount;
    char    szCharacterName[50][32];
    int32_t iLevel[50];
};

struct ItemListMix {
    int32_t iTypeMix;
    int32_t iLucidy;
    int32_t iSereneo;
    int32_t iFadeo;
    int32_t iSparky;
    int32_t iRaident;
    int32_t iTransparo;
    int32_t iMurky;
    int32_t iDevine;
    int32_t iCelesto;
    int32_t iMirage;
    int32_t iInferna;
    int32_t iEnigma;
    int32_t iBellum;
};

struct ItemMixDesc {
    char Description[32];
};

struct PacketMixGuild {
    PacketHeader header;
    int32_t     iCount;
    ItemListMix sMixList[70]; // Usando a constante MIXLIST_FUNCTION_TOTAL
    ItemMixDesc sMixDesc[70];
};

struct PacketSettingsData {
    PacketHeader header;
    BOOL    bInital;
    BOOL    bNoTradeChat;
    BOOL    bNoPartyRequest;
    BOOL    bNoTradeRequest;
    BOOL    bNoWhisper;
    BYTE    bWeaponEnchantPerference;
};

struct PacketCrash {
    PacketHeader header;
    DWORD   dwExceptionCode;
    DWORD   dwExceptionFlags;
    PVOID   pvExceptionAddress;
    DWORD   EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI, EIP;
    DWORD   dwVersion;
    char    szAccountName[32];
};

struct PacketFireMeteorite {
    PacketHeader header;
    Point3D sBegin;
    Point3D sEnd;
    int32_t iDelay;
    BOOL    bSmallMeteor;
    int32_t iCount;
};

struct PacketPing {
    PacketHeader header;
    uint32_t     dwTick; // Usado pelo servidor legacy, podemos manter por compatibilidade
    uint32_t     dwTime; // Usado pelo cliente para o cálculo do tempo
    BOOL         CheatEngineDetected;
};

struct ServerListServerInfo {
    char    szName[32];
    char    szaIP[3][20];
    int32_t iaPort[4];
};

struct PacketServerList {
    PacketHeader header;
    char         szServerName[16];
    uint32_t     dwTime;
    int32_t      iTicket;
    uint32_t     dwUnknown;
    int32_t      iClanServerIndex;
    int32_t      iGameServers;
    ServerListServerInfo sServers[4]; // Suporta até 4 servidores de jogo
};

#pragma pack(pop)