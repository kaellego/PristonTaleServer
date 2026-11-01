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
    Experience bigint NOT NULL DEFAULT 0
);
GO

USE GameDB;
GO
IF OBJECT_ID('dbo.ItemList', 'U') IS NULL
CREATE TABLE dbo.ItemList (
    ItemCode int NOT NULL PRIMARY KEY,
    ItemName varchar(63) NOT NULL,
    RequiredLevel int NOT NULL DEFAULT 1,
    EquipmentSlot int NOT NULL DEFAULT -2,
    JobCodeMask bigint NOT NULL DEFAULT 0,
    Consumable bit NOT NULL DEFAULT 0
);
IF COL_LENGTH('dbo.ItemList', 'RequiredLevel') IS NULL ALTER TABLE dbo.ItemList ADD RequiredLevel int NOT NULL CONSTRAINT DF_ItemList_RequiredLevel DEFAULT 1;
IF COL_LENGTH('dbo.ItemList', 'EquipmentSlot') IS NULL ALTER TABLE dbo.ItemList ADD EquipmentSlot int NOT NULL CONSTRAINT DF_ItemList_EquipmentSlot DEFAULT -2;
IF COL_LENGTH('dbo.ItemList', 'JobCodeMask') IS NULL ALTER TABLE dbo.ItemList ADD JobCodeMask bigint NOT NULL CONSTRAINT DF_ItemList_JobCodeMask DEFAULT 0;
IF COL_LENGTH('dbo.ItemList', 'Consumable') IS NULL ALTER TABLE dbo.ItemList ADD Consumable bit NOT NULL CONSTRAINT DF_ItemList_Consumable DEFAULT 0;
-- Fail closed: legacy rows without a reviewed definition cannot be equipped.
UPDATE dbo.ItemList
SET RequiredLevel = 1, EquipmentSlot = -2, JobCodeMask = 0, Consumable = 0
WHERE EquipmentSlot = -1 AND JobCodeMask = 4294967295;
IF NOT EXISTS (SELECT 1 FROM dbo.ItemList WHERE ItemCode=1)
    INSERT INTO dbo.ItemList (ItemCode, ItemName, EquipmentSlot) VALUES (1, 'Beginner Sword', 0);
UPDATE dbo.ItemList
SET RequiredLevel = 1, EquipmentSlot = 0, JobCodeMask = 4294967295, Consumable = 0
WHERE ItemCode = 1;
IF OBJECT_ID('dbo.GameMasters', 'U') IS NULL
CREATE TABLE dbo.GameMasters (ID int IDENTITY PRIMARY KEY, AccountID varchar(32), Password varchar(30), Type int NOT NULL DEFAULT 0, active bit NOT NULL DEFAULT 1);
GO

USE LogDB;
GO
IF OBJECT_ID('dbo.ServerLog', 'U') IS NULL
CREATE TABLE dbo.ServerLog (ID bigint IDENTITY PRIMARY KEY, CreatedAt datetime2 NOT NULL DEFAULT sysdatetime(), Message nvarchar(max) NULL);
GO
