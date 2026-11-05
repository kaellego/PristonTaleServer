-- Execute once in the SQL Server instance used by RustPT.
USE UserDB;
IF COL_LENGTH('dbo.CharacterInfo', 'Gold') IS NULL
    ALTER TABLE dbo.CharacterInfo ADD Gold bigint NOT NULL CONSTRAINT DF_CharacterInfo_Gold DEFAULT 0;

IF COL_LENGTH('dbo.CharacterInfo', 'MapId') IS NULL
    ALTER TABLE dbo.CharacterInfo ADD MapId int NOT NULL CONSTRAINT DF_CharacterInfo_MapId DEFAULT 0;
IF COL_LENGTH('dbo.CharacterInfo', 'PosX') IS NULL
    ALTER TABLE dbo.CharacterInfo ADD PosX int NOT NULL CONSTRAINT DF_CharacterInfo_PosX DEFAULT 0;
IF COL_LENGTH('dbo.CharacterInfo', 'PosY') IS NULL
    ALTER TABLE dbo.CharacterInfo ADD PosY int NOT NULL CONSTRAINT DF_CharacterInfo_PosY DEFAULT 0;
IF COL_LENGTH('dbo.CharacterInfo', 'PosZ') IS NULL
    ALTER TABLE dbo.CharacterInfo ADD PosZ int NOT NULL CONSTRAINT DF_CharacterInfo_PosZ DEFAULT 0;
IF COL_LENGTH('dbo.CharacterInfo', 'ClassId') IS NULL
    ALTER TABLE dbo.CharacterInfo ADD ClassId int NOT NULL CONSTRAINT DF_CharacterInfo_ClassId DEFAULT 1;
IF COL_LENGTH('dbo.CharacterInfo', 'Level') IS NULL
    ALTER TABLE dbo.CharacterInfo ADD Level int NOT NULL CONSTRAINT DF_CharacterInfo_Level DEFAULT 1;

IF OBJECT_ID('dbo.SecurityLoginAudit', 'U') IS NULL
CREATE TABLE dbo.SecurityLoginAudit (
    AuditId bigint IDENTITY PRIMARY KEY,
    AccountName varchar(32) NULL,
    IpAddress varchar(45) NOT NULL,
    DeviceHash char(64) NOT NULL,
    Succeeded bit NOT NULL,
    FailureReason varchar(64) NOT NULL,
    CreatedAt datetime2 NOT NULL CONSTRAINT DF_SecurityLoginAudit_CreatedAt DEFAULT sysdatetime()
);
CREATE INDEX IX_SecurityLoginAudit_AccountCreated ON dbo.SecurityLoginAudit (AccountName, CreatedAt DESC);
CREATE INDEX IX_SecurityLoginAudit_IpCreated ON dbo.SecurityLoginAudit (IpAddress, CreatedAt DESC);
GO
