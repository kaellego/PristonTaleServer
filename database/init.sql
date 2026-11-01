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
    ItemName varchar(63) NOT NULL
);
IF NOT EXISTS (SELECT 1 FROM dbo.ItemList WHERE ItemCode=1)
    INSERT INTO dbo.ItemList (ItemCode, ItemName) VALUES (1, 'Beginner Sword');
IF OBJECT_ID('dbo.GameMasters', 'U') IS NULL
CREATE TABLE dbo.GameMasters (ID int IDENTITY PRIMARY KEY, AccountID varchar(32), Password varchar(30), Type int NOT NULL DEFAULT 0, active bit NOT NULL DEFAULT 1);
GO

USE LogDB;
GO
IF OBJECT_ID('dbo.ServerLog', 'U') IS NULL
CREATE TABLE dbo.ServerLog (ID bigint IDENTITY PRIMARY KEY, CreatedAt datetime2 NOT NULL DEFAULT sysdatetime(), Message nvarchar(max) NULL);
GO
