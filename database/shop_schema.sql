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
GO
