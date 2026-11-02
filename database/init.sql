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
GO

USE LogDB;
GO
IF OBJECT_ID('dbo.ServerLog', 'U') IS NULL
CREATE TABLE dbo.ServerLog (ID bigint IDENTITY PRIMARY KEY, CreatedAt datetime2 NOT NULL DEFAULT sysdatetime(), Message nvarchar(max) NULL);
GO
