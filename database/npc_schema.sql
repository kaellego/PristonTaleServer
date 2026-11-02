USE GameDB;
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
GO

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
    CONSTRAINT CK_NpcSpawn_EnabledCoordinates CHECK (
        Enabled = 0 OR (X IS NOT NULL AND Y IS NOT NULL AND Z IS NOT NULL)
    )
);
GO

IF COL_LENGTH('dbo.NpcList', 'SourceFile') IS NULL
    ALTER TABLE dbo.NpcList ADD SourceFile varchar(260) NULL;
GO
