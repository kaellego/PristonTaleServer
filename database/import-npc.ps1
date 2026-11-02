[CmdletBinding()]
param(
    [string]$NpcPath = 'C:\Users\weeel\Desktop\ZeroPT-Server\GameServer\NPC',
    [string]$FieldPath = 'C:\Users\weeel\Desktop\ZeroPT-Server\GameServer\Field',
    [string]$MapNpcCsv = '',
    [string]$Container = 'pristontale-sqlserver',
    [string]$Password = 'PristonDev_2026!'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $NpcPath)) { throw "Diretorio de NPC nao encontrado: $NpcPath" }
if (-not (Test-Path -LiteralPath $FieldPath)) { throw "Diretorio de mapas nao encontrado: $FieldPath" }

function SqlString([string]$value) {
    if ($null -eq $value) { return 'NULL' }
    return "N'" + $value.Replace("'", "''") + "'"
}

function Get-NpcDefinition([System.IO.FileInfo]$file, [int]$id) {
    $bytes = [IO.File]::ReadAllBytes($file.FullName)
    $ascii = [Text.Encoding]::ASCII.GetString($bytes)
    $modelMatch = [regex]::Match($ascii, 'char\\npc\\[^"\x00\r\n]+?\.(?:ini|smd)', [Text.RegularExpressions.RegexOptions]::IgnoreCase)
    $model = if ($modelMatch.Success) { $modelMatch.Value -replace '(?i)\.ini$', '.smd' } else { '' }
    [pscustomobject]@{
        Id = $id
        File = $file.Name
        Name = $file.BaseName
        Model = $model
        Source = $file.FullName
    }
}

$files = @(Get-ChildItem -LiteralPath $NpcPath -File | Where-Object { $_.Extension -match '^\.npc$' } | Sort-Object Name)
if ($files.Count -eq 0) { throw 'Nenhum arquivo .NPC foi encontrado.' }

$definitions = @()
$idsByFile = @{}
for ($i = 0; $i -lt $files.Count; $i++) {
    $definition = Get-NpcDefinition $files[$i] (10000 + $i)
    $definitions += $definition
    $idsByFile[$definition.File.ToLowerInvariant()] = $definition.Id
}

# Map IDs that are stable in the Fortress/Priston map enum. Unknown maps stay -1
# and their instances remain disabled until MapNPC coordinates are supplied.
$mapIds = @{
    'fore-3.ase' = 0; 'fore-2.ase' = 1; 'village-2.ase' = 3; 'ruin-2.ase' = 6;
    'village-1.ase' = 9; 'pilai.ase' = 21; 'ice_ura.ase' = 29; 'iron-1.ase' = 44;
    'old_village-2.ase' = 45
}

$coordinateRows = @{}
if ($MapNpcCsv) {
    if (-not (Test-Path -LiteralPath $MapNpcCsv)) { throw "CSV de coordenadas nao encontrado: $MapNpcCsv" }
    foreach ($row in (Import-Csv -LiteralPath $MapNpcCsv)) {
        $key = "$($row.SourceMap.ToLowerInvariant())|$($row.LegacyFile.ToLowerInvariant())"
        $coordinateRows[$key] = $row
    }
}

$spawns = @()
foreach ($field in (Get-ChildItem -LiteralPath $FieldPath -File -Filter '*.spc')) {
    $ascii = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($field.FullName))
    foreach ($match in [regex]::Matches($ascii, 'GameServer\\npc\\([^\x00" ]+\.(?:npc|NPC))', [Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
        $legacyFile = $match.Groups[1].Value
        $id = $idsByFile[$legacyFile.ToLowerInvariant()]
        if (-not $id) { continue }
        $sourceMap = $field.BaseName.ToLowerInvariant()
        $mapId = if ($mapIds.ContainsKey($sourceMap)) { $mapIds[$sourceMap] } else { -1 }
        $coordinates = $coordinateRows["$sourceMap|$($legacyFile.ToLowerInvariant())"]
        $enabled = 0
        $x = $null; $y = $null; $z = $null; $angle = 0
        if ($coordinates) {
            $x = [int]$coordinates.X; $y = [int]$coordinates.Y; $z = [int]$coordinates.Z
            $angle = [int]$coordinates.AngleY
            $enabled = if ([int]$coordinates.Enabled -ne 0 -and $mapId -ge 0) { 1 } else { 0 }
        }
        $spawns += [pscustomobject]@{ Id = $id; MapId = $mapId; SourceMap = $sourceMap; X = $x; Y = $y; Z = $z; Angle = $angle; Enabled = $enabled }
    }
}
$spawns = @($spawns | Sort-Object Id, MapId, SourceMap -Unique)

$sql = [Text.StringBuilder]::new()
[void]$sql.AppendLine('USE GameDB;')
[void]$sql.AppendLine('SET XACT_ABORT ON; BEGIN TRANSACTION;')
[void]$sql.AppendLine((Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'npc_schema.sql')))
foreach ($npc in $definitions) {
    [void]$sql.AppendLine("IF EXISTS (SELECT 1 FROM dbo.NpcList WHERE NpcId=$($npc.Id)) UPDATE dbo.NpcList SET LegacyFile=$(SqlString $npc.File), DisplayName=$(SqlString $npc.Name), ModelName=$(SqlString $npc.Model), SourceFile=$(SqlString $npc.Source) WHERE NpcId=$($npc.Id) ELSE INSERT INTO dbo.NpcList (NpcId, LegacyFile, DisplayName, ModelName, SourceFile) VALUES ($($npc.Id), $(SqlString $npc.File), $(SqlString $npc.Name), $(SqlString $npc.Model), $(SqlString $npc.Source));")
}
foreach ($spawn in $spawns) {
    $enabled = if ($spawn.Enabled -eq 1) { 1 } else { 0 }
    $x = if ($null -eq $spawn.X) { 'NULL' } else { $spawn.X }
    $y = if ($null -eq $spawn.Y) { 'NULL' } else { $spawn.Y }
    $z = if ($null -eq $spawn.Z) { 'NULL' } else { $spawn.Z }
    [void]$sql.AppendLine("IF EXISTS (SELECT 1 FROM dbo.NpcSpawn WHERE NpcId=$($spawn.Id) AND MapId=$($spawn.MapId) AND SourceMap=$(SqlString $spawn.SourceMap)) UPDATE dbo.NpcSpawn SET X=$x, Y=$y, Z=$z, AngleY=$($spawn.Angle), Enabled=$enabled WHERE NpcId=$($spawn.Id) AND MapId=$($spawn.MapId) AND SourceMap=$(SqlString $spawn.SourceMap) ELSE INSERT INTO dbo.NpcSpawn (NpcId, MapId, SourceMap, X, Y, Z, AngleY, Enabled) VALUES ($($spawn.Id), $($spawn.MapId), $(SqlString $spawn.SourceMap), $x, $y, $z, $($spawn.Angle), $enabled);")
}
[void]$sql.AppendLine('COMMIT TRANSACTION;')

$temporarySql = Join-Path ([IO.Path]::GetTempPath()) 'pristontale-npc-import.sql'
[IO.File]::WriteAllText($temporarySql, $sql.ToString(), [Text.UTF8Encoding]::new($false))
try {
    Get-Content -Raw -LiteralPath $temporarySql | docker exec -i $Container /opt/mssql-tools18/bin/sqlcmd -S localhost -U sa -P $Password -C -b
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao importar o catalogo de NPCs.' }
} finally {
    Remove-Item -LiteralPath $temporarySql -Force -ErrorAction SilentlyContinue
}

$enabledCount = @($spawns | Where-Object Enabled -eq 1).Count
Write-Host "NPCs importados: $($definitions.Count); instancias de mapa: $($spawns.Count); instancias habilitadas: $enabledCount"
if ($enabledCount -eq 0) {
    Write-Warning 'Nenhuma instancia foi habilitada: os backups fornecidos nao possuem coordenadas MapNPC. Forneca -MapNpcCsv para ativar spawns reais.'
}
