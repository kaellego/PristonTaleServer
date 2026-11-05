-- Execute once in the SQL Server instance used by RustPT.
USE UserDB;
IF COL_LENGTH('dbo.CharacterInfo', 'Gold') IS NULL
    ALTER TABLE dbo.CharacterInfo ADD Gold bigint NOT NULL CONSTRAINT DF_CharacterInfo_Gold DEFAULT 0;
GO
