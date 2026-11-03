SET ANSI_NULLS ON;
SET QUOTED_IDENTIFIER ON;
SET ANSI_PADDING ON;
SET ANSI_WARNINGS ON;
SET CONCAT_NULL_YIELDS_NULL ON;
SET ARITHABORT ON;
SET NUMERIC_ROUNDABORT OFF;
SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF DB_ID('UserDB') IS NULL CREATE DATABASE UserDB;
IF DB_ID('GameDB') IS NULL CREATE DATABASE GameDB;
IF DB_ID('LogDB') IS NULL CREATE DATABASE LogDB;
GO

USE UserDB;
GO
IF OBJECT_ID('dbo.UserInfo', 'U') IS NULL
CREATE TABLE dbo.UserInfo (
    ID int IDENTITY PRIMARY KEY,
    AccountName varchar(32) NOT NULL UNIQUE,
    Password varchar(65) NOT NULL,
    Flag int NOT NULL DEFAULT 0,
    BanStatus int NOT NULL DEFAULT 0,
    IsMuted bit NOT NULL DEFAULT 0
);
IF OBJECT_ID('dbo.CharacterInfo', 'U') IS NULL
CREATE TABLE dbo.CharacterInfo (
    Name varchar(31) PRIMARY KEY,
    AccountName varchar(32) NOT NULL,
    Experience bigint NOT NULL DEFAULT 0,
    StateVersion int NOT NULL DEFAULT 1,
    StateUpdatedAt datetime2 NOT NULL DEFAULT sysdatetime()
);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.CharacterInfo') AND name = 'IX_CharacterInfo_Account_Experience')
    CREATE INDEX IX_CharacterInfo_Account_Experience
        ON dbo.CharacterInfo (AccountName, Experience DESC, Name)
        INCLUDE (StateVersion, StateUpdatedAt);
IF COL_LENGTH('dbo.CharacterInfo', 'StateVersion') IS NULL ALTER TABLE dbo.CharacterInfo ADD StateVersion int NOT NULL CONSTRAINT DF_CharacterInfo_StateVersion DEFAULT 1;
IF COL_LENGTH('dbo.CharacterInfo', 'StateUpdatedAt') IS NULL ALTER TABLE dbo.CharacterInfo ADD StateUpdatedAt datetime2 NOT NULL CONSTRAINT DF_CharacterInfo_StateUpdatedAt DEFAULT sysdatetime();
IF OBJECT_ID('dbo.CharacterInventory', 'U') IS NULL
CREATE TABLE dbo.CharacterInventory (
    CharacterName varchar(31) NOT NULL,
    SlotIndex int NOT NULL,
    ItemCode int NOT NULL,
    ItemName varchar(64) NOT NULL DEFAULT '',
    Quantity int NOT NULL DEFAULT 1,
    ItemSerial bigint NOT NULL DEFAULT 0,
    ItemChecksum int NOT NULL DEFAULT 0,
    ItemVersion int NOT NULL DEFAULT 1,
    CONSTRAINT PK_CharacterInventory PRIMARY KEY (CharacterName, SlotIndex),
    CONSTRAINT CK_CharacterInventory_Slot CHECK (SlotIndex >= 0 AND SlotIndex < 60),
    CONSTRAINT CK_CharacterInventory_Quantity CHECK (Quantity > 0)
);
IF OBJECT_ID('dbo.AccountWarehouse', 'U') IS NULL
CREATE TABLE dbo.AccountWarehouse (
    AccountName varchar(32) NOT NULL,
    SlotIndex int NOT NULL,
    ItemCode int NOT NULL,
    ItemName varchar(64) NOT NULL DEFAULT '',
    Quantity int NOT NULL DEFAULT 1,
    ItemSerial bigint NOT NULL DEFAULT 0,
    ItemChecksum int NOT NULL DEFAULT 0,
    ItemVersion int NOT NULL DEFAULT 1,
    CONSTRAINT PK_AccountWarehouse PRIMARY KEY (AccountName, SlotIndex),
    CONSTRAINT CK_AccountWarehouse_Slot CHECK (SlotIndex >= 0 AND SlotIndex < 405),
    CONSTRAINT CK_AccountWarehouse_Quantity CHECK (Quantity > 0)
);
IF OBJECT_ID('dbo.PlayerStateProto', 'U') IS NULL
CREATE TABLE dbo.PlayerStateProto (
    StateType tinyint NOT NULL,
    OwnerKey varchar(32) NOT NULL,
    ChunkIndex int NOT NULL,
    ChunkData varbinary(max) NOT NULL,
    StateVersion int NOT NULL DEFAULT 1,
    StateMac varbinary(32) NULL,
    StateUpdatedAt datetime2 NOT NULL DEFAULT sysdatetime(),
    CONSTRAINT PK_PlayerStateProto PRIMARY KEY (StateType, OwnerKey, ChunkIndex),
    CONSTRAINT CK_PlayerStateProto_Type CHECK (StateType IN (1, 2, 3)),
    CONSTRAINT CK_PlayerStateProto_Chunk CHECK (ChunkIndex >= 0)
);
IF EXISTS (
    SELECT 1 FROM sys.columns
    WHERE object_id = OBJECT_ID('dbo.PlayerStateProto')
      AND name = 'ChunkData'
      AND system_type_id IN (167, 175, 231, 239)
)
BEGIN
    EXEC(N'ALTER TABLE dbo.PlayerStateProto ADD ChunkDataBinary varbinary(max) NULL');
    EXEC(N'UPDATE dbo.PlayerStateProto SET ChunkDataBinary = CONVERT(varbinary(max), ChunkData)');
    EXEC(N'ALTER TABLE dbo.PlayerStateProto ALTER COLUMN ChunkDataBinary varbinary(max) NOT NULL');
    EXEC(N'ALTER TABLE dbo.PlayerStateProto DROP COLUMN ChunkData');
    EXEC(N'EXEC sp_rename ''dbo.PlayerStateProto.ChunkDataBinary'', ''ChunkData'', ''COLUMN''');
END;
IF COL_LENGTH('dbo.PlayerStateProto', 'StateMac') IS NULL
    ALTER TABLE dbo.PlayerStateProto ADD StateMac varbinary(32) NULL;
IF COL_LENGTH('dbo.CharacterInventory', 'ItemSerial') IS NULL ALTER TABLE dbo.CharacterInventory ADD ItemSerial bigint NOT NULL CONSTRAINT DF_CharacterInventory_ItemSerial DEFAULT 0;
IF COL_LENGTH('dbo.CharacterInventory', 'ItemChecksum') IS NULL ALTER TABLE dbo.CharacterInventory ADD ItemChecksum int NOT NULL CONSTRAINT DF_CharacterInventory_ItemChecksum DEFAULT 0;
IF COL_LENGTH('dbo.CharacterInventory', 'ItemVersion') IS NULL ALTER TABLE dbo.CharacterInventory ADD ItemVersion int NOT NULL CONSTRAINT DF_CharacterInventory_ItemVersion DEFAULT 1;
IF COL_LENGTH('dbo.AccountWarehouse', 'ItemSerial') IS NULL ALTER TABLE dbo.AccountWarehouse ADD ItemSerial bigint NOT NULL CONSTRAINT DF_AccountWarehouse_ItemSerial DEFAULT 0;
IF COL_LENGTH('dbo.AccountWarehouse', 'ItemChecksum') IS NULL ALTER TABLE dbo.AccountWarehouse ADD ItemChecksum int NOT NULL CONSTRAINT DF_AccountWarehouse_ItemChecksum DEFAULT 0;
IF COL_LENGTH('dbo.AccountWarehouse', 'ItemVersion') IS NULL ALTER TABLE dbo.AccountWarehouse ADD ItemVersion int NOT NULL CONSTRAINT DF_AccountWarehouse_ItemVersion DEFAULT 1;
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.PlayerStateProto') AND name = 'IX_PlayerStateProto_Owner')
    CREATE INDEX IX_PlayerStateProto_Owner ON dbo.PlayerStateProto (OwnerKey, StateType, ChunkIndex) INCLUDE (StateVersion, StateUpdatedAt);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.CharacterInventory') AND name = 'IX_CharacterInventory_ItemCode')
    CREATE INDEX IX_CharacterInventory_ItemCode ON dbo.CharacterInventory (ItemCode, CharacterName) INCLUDE (SlotIndex, Quantity, ItemSerial, ItemVersion);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.AccountWarehouse') AND name = 'IX_AccountWarehouse_ItemCode')
    CREATE INDEX IX_AccountWarehouse_ItemCode ON dbo.AccountWarehouse (ItemCode, AccountName) INCLUDE (SlotIndex, Quantity, ItemSerial, ItemVersion);
GO

USE GameDB;
GO
IF OBJECT_ID('dbo.ItemList', 'U') IS NULL
CREATE TABLE dbo.ItemList (
    ItemCode int NOT NULL PRIMARY KEY,
    LegacyCode varchar(32) NULL,
    ItemName nvarchar(63) NOT NULL,
    RequiredLevel int NOT NULL DEFAULT 1,
    EquipmentSlot int NOT NULL DEFAULT -2,
    JobCodeMask bigint NOT NULL DEFAULT 0,
    Consumable bit NOT NULL DEFAULT 0
);
IF COL_LENGTH('dbo.ItemList', 'RequiredLevel') IS NULL ALTER TABLE dbo.ItemList ADD RequiredLevel int NOT NULL CONSTRAINT DF_ItemList_RequiredLevel DEFAULT 1;
IF COL_LENGTH('dbo.ItemList', 'LegacyCode') IS NULL ALTER TABLE dbo.ItemList ADD LegacyCode varchar(32) NULL;
IF COL_LENGTH('dbo.ItemList', 'EquipmentSlot') IS NULL ALTER TABLE dbo.ItemList ADD EquipmentSlot int NOT NULL CONSTRAINT DF_ItemList_EquipmentSlot DEFAULT -2;
IF COL_LENGTH('dbo.ItemList', 'JobCodeMask') IS NULL ALTER TABLE dbo.ItemList ADD JobCodeMask bigint NOT NULL CONSTRAINT DF_ItemList_JobCodeMask DEFAULT 0;
IF COL_LENGTH('dbo.ItemList', 'Consumable') IS NULL ALTER TABLE dbo.ItemList ADD Consumable bit NOT NULL CONSTRAINT DF_ItemList_Consumable DEFAULT 0;
GO
USE GameDB;
GO
IF EXISTS (SELECT 1 FROM sys.columns WHERE object_id=OBJECT_ID('dbo.ItemList') AND name='ItemName' AND system_type_id=167)
    ALTER TABLE dbo.ItemList ALTER COLUMN ItemName nvarchar(63) NOT NULL;
GO
USE GameDB;
GO
-- Fail closed: legacy rows without a reviewed definition cannot be equipped.
UPDATE dbo.ItemList
SET RequiredLevel = 1, EquipmentSlot = -2, JobCodeMask = 0, Consumable = 0
WHERE EquipmentSlot = -1 AND JobCodeMask = 4294967295;
IF NOT EXISTS (SELECT 1 FROM dbo.ItemList WHERE ItemCode=1)
    INSERT INTO dbo.ItemList (ItemCode, LegacyCode, ItemName, EquipmentSlot) VALUES (1, 'BEGINNER', 'Beginner Sword', 0);
UPDATE dbo.ItemList
SET LegacyCode = 'BEGINNER', RequiredLevel = 1, EquipmentSlot = 0, JobCodeMask = 4294967295, Consumable = 0
WHERE ItemCode = 1;
IF OBJECT_ID('dbo.GameMasters', 'U') IS NULL
CREATE TABLE dbo.GameMasters (ID int IDENTITY PRIMARY KEY, AccountID varchar(32), Password varchar(30), Type int NOT NULL DEFAULT 0, active bit NOT NULL DEFAULT 1);
IF COL_LENGTH('dbo.GameMasters', 'UserID') IS NULL
    ALTER TABLE dbo.GameMasters ADD UserID int NULL;
GO
-- UserInfo lives in UserDB, so SQL Server cannot enforce a cross-database FK here.
-- Backfill the stable numeric identity while retaining AccountID during migration.
UPDATE gm
   SET gm.UserID = ui.ID
  FROM dbo.GameMasters AS gm
  INNER JOIN UserDB.dbo.UserInfo AS ui ON ui.AccountName = gm.AccountID
 WHERE gm.UserID IS NULL;
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.GameMasters') AND name = 'UX_GameMasters_ActiveAccount')
    CREATE UNIQUE INDEX UX_GameMasters_ActiveAccount ON dbo.GameMasters (AccountID) WHERE active = 1 AND AccountID IS NOT NULL;
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.GameMasters') AND name = 'UX_GameMasters_ActiveUserID')
    CREATE UNIQUE INDEX UX_GameMasters_ActiveUserID ON dbo.GameMasters (UserID) WHERE active = 1 AND UserID IS NOT NULL;
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.GameMasters') AND name = 'IX_GameMasters_Account_Active')
    CREATE INDEX IX_GameMasters_Account_Active ON dbo.GameMasters (AccountID, active) INCLUDE (Type);
GO

IF OBJECT_ID('dbo.NpcList', 'U') IS NULL
CREATE TABLE dbo.NpcList (
    NpcId int NOT NULL PRIMARY KEY,
    LegacyFile varchar(128) NOT NULL UNIQUE,
    DisplayName nvarchar(63) NOT NULL,
    ModelName varchar(127) NOT NULL,
    EventCode int NOT NULL DEFAULT 0,
    Enabled bit NOT NULL DEFAULT 1,
    SourceFile varchar(260) NULL
);
IF OBJECT_ID('dbo.NpcSpawn', 'U') IS NULL
CREATE TABLE dbo.NpcSpawn (
    SpawnId int IDENTITY(1, 1) NOT NULL PRIMARY KEY,
    NpcId int NOT NULL,
    MapId int NOT NULL,
    SourceMap varchar(128) NOT NULL,
    X int NULL,
    Y int NULL,
    Z int NULL,
    AngleY int NOT NULL DEFAULT 0,
    Enabled bit NOT NULL DEFAULT 0,
    CONSTRAINT FK_NpcSpawn_NpcList FOREIGN KEY (NpcId) REFERENCES dbo.NpcList(NpcId),
    CONSTRAINT UQ_NpcSpawn_Source UNIQUE (NpcId, MapId, SourceMap),
    CONSTRAINT CK_NpcSpawn_EnabledCoordinates CHECK (Enabled = 0 OR (X IS NOT NULL AND Y IS NOT NULL AND Z IS NOT NULL))
);
IF OBJECT_ID('dbo.NpcShop', 'U') IS NULL
CREATE TABLE dbo.NpcShop (
    ShopId int IDENTITY(1, 1) NOT NULL PRIMARY KEY,
    NpcId int NOT NULL,
    ShopName nvarchar(63) NOT NULL,
    Enabled bit NOT NULL DEFAULT 1,
    CONSTRAINT FK_NpcShop_NpcList FOREIGN KEY (NpcId) REFERENCES dbo.NpcList(NpcId),
    CONSTRAINT UQ_NpcShop_Npc UNIQUE (NpcId)
);
IF OBJECT_ID('dbo.NpcShopItem', 'U') IS NULL
CREATE TABLE dbo.NpcShopItem (
    ShopId int NOT NULL,
    ItemCode int NOT NULL,
    BuyPrice bigint NOT NULL,
    SellPrice bigint NOT NULL DEFAULT 0,
    Stock int NULL,
    MaxQuantity int NOT NULL DEFAULT 2000000000,
    RequiredLevel int NOT NULL DEFAULT 1,
    Enabled bit NOT NULL DEFAULT 1,
    CONSTRAINT PK_NpcShopItem PRIMARY KEY (ShopId, ItemCode),
    CONSTRAINT FK_NpcShopItem_Shop FOREIGN KEY (ShopId) REFERENCES dbo.NpcShop(ShopId),
    CONSTRAINT FK_NpcShopItem_Item FOREIGN KEY (ItemCode) REFERENCES dbo.ItemList(ItemCode),
    CONSTRAINT CK_NpcShopItem_Prices CHECK (BuyPrice >= 0 AND SellPrice >= 0),
    CONSTRAINT CK_NpcShopItem_Quantity CHECK (MaxQuantity > 0 AND (Stock IS NULL OR Stock >= 0))
);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.NpcShopItem') AND name = 'IX_NpcShopItem_Enabled')
    CREATE INDEX IX_NpcShopItem_Enabled ON dbo.NpcShopItem (ShopId, Enabled, ItemCode);
IF OBJECT_ID('dbo.NpcDrop', 'U') IS NULL
CREATE TABLE dbo.NpcDrop (
    NpcId int NOT NULL,
    ItemCode int NOT NULL,
    DropChancePermille int NOT NULL,
    MinQuantity int NOT NULL DEFAULT 1,
    MaxQuantity int NOT NULL DEFAULT 1,
    Enabled bit NOT NULL DEFAULT 1,
    CONSTRAINT PK_NpcDrop PRIMARY KEY (NpcId, ItemCode),
    CONSTRAINT FK_NpcDrop_NpcList FOREIGN KEY (NpcId) REFERENCES dbo.NpcList(NpcId),
    CONSTRAINT FK_NpcDrop_Item FOREIGN KEY (ItemCode) REFERENCES dbo.ItemList(ItemCode),
    CONSTRAINT CK_NpcDrop_Chance CHECK (DropChancePermille BETWEEN 0 AND 1000),
    CONSTRAINT CK_NpcDrop_Quantity CHECK (MinQuantity > 0 AND MaxQuantity >= MinQuantity)
);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.NpcDrop') AND name = 'IX_NpcDrop_Enabled')
    CREATE INDEX IX_NpcDrop_Enabled ON dbo.NpcDrop (NpcId, Enabled);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.ItemList') AND name = 'IX_ItemList_LegacyCode')
    CREATE INDEX IX_ItemList_LegacyCode ON dbo.ItemList (LegacyCode) INCLUDE (ItemName, RequiredLevel, EquipmentSlot, JobCodeMask, Consumable);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.NpcList') AND name = 'IX_NpcList_Enabled')
    CREATE INDEX IX_NpcList_Enabled ON dbo.NpcList (Enabled, NpcId) INCLUDE (DisplayName, ModelName, EventCode);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.NpcSpawn') AND name = 'IX_NpcSpawn_EnabledMap')
    CREATE INDEX IX_NpcSpawn_EnabledMap ON dbo.NpcSpawn (Enabled, MapId, NpcId) INCLUDE (SpawnId, X, Y, Z, AngleY);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.NpcShop') AND name = 'IX_NpcShop_EnabledNpc')
    CREATE INDEX IX_NpcShop_EnabledNpc ON dbo.NpcShop (Enabled, NpcId) INCLUDE (ShopId, ShopName);
GO

USE LogDB;
GO
IF OBJECT_ID('dbo.ServerLog', 'U') IS NULL
CREATE TABLE dbo.ServerLog (ID bigint IDENTITY PRIMARY KEY, CreatedAt datetime2 NOT NULL DEFAULT sysdatetime(), Message nvarchar(max) NULL);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID('dbo.ServerLog') AND name = 'IX_ServerLog_CreatedAt')
    CREATE INDEX IX_ServerLog_CreatedAt ON dbo.ServerLog (CreatedAt DESC) INCLUDE (Message);
GO
