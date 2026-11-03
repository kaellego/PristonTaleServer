[CmdletBinding()]
param(
    [string]$NpcPath = 'C:\Users\weeel\Desktop\ZeroPT-Server\GameServer\NPC',
    [string]$FieldPath = 'C:\Users\weeel\Desktop\ZeroPT-Server\GameServer\Field',
    [string]$OpenItemPath = 'C:\Users\weeel\Desktop\ZeroPT-Server\GameServer\OpenItem',
    [string]$MapNpcCsv = '',
    [int]$MapNpcCoordinateScale = 256,
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
    $modelMatch = [regex]::Match($ascii, 'char\\npc\\[^"\x00\r\n]+?\.(?:ini|inx|smd)', [Text.RegularExpressions.RegexOptions]::IgnoreCase)
    # The client receives the .inx index; it resolves the corresponding .smd.
    $model = if ($modelMatch.Success) { $modelMatch.Value -replace '(?i)\.(?:ini|smd)$', '.inx' } else { '' }
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

$itemPrices = @{}
if (Test-Path -LiteralPath $OpenItemPath) {
    foreach ($itemFile in (Get-ChildItem -LiteralPath $OpenItemPath -File -Filter '*.txt')) {
        $itemText = [Text.Encoding]::GetEncoding(949).GetString([IO.File]::ReadAllBytes($itemFile.FullName))
        $code = $itemFile.BaseName.ToUpperInvariant()
        $priceMatch = [regex]::Match($itemText, '(?m)^\*가격\s+(\d+)\s*$')
        if ($priceMatch.Success) { $itemPrices[$code] = [int64]$priceMatch.Groups[1].Value }
    }
}

# Decode prices by position: in OpenItem files the first numeric attribute is
# weight and the next one is the authoritative NPC price. This avoids relying
# on the original Korean field label when PowerShell uses another code page.
$itemPrices = @{}
foreach ($itemFile in (Get-ChildItem -LiteralPath $OpenItemPath -File -Filter '*.txt')) {
    $itemText = [Text.Encoding]::GetEncoding(949).GetString([IO.File]::ReadAllBytes($itemFile.FullName))
    $weightSeen = $false
    foreach ($itemLine in ($itemText -split "`r?`n")) {
        $valueMatch = [regex]::Match($itemLine, '^\*[^\d\r\n]*\s+(\d+)\s*$')
        if (-not $valueMatch.Success) { continue }
        if (-not $weightSeen) { $weightSeen = $true; continue }
        $itemPrices[$itemFile.BaseName.ToUpperInvariant()] = [int64]$valueMatch.Groups[1].Value
        break
    }
}

$definitions = @()
$idsByFile = @{}
for ($i = 0; $i -lt $files.Count; $i++) {
    $definition = Get-NpcDefinition $files[$i] (10000 + $i)
    $definitions += $definition
    $idsByFile[$definition.File.ToLowerInvariant()] = $definition.Id
}

# NPC files are the authoritative shop catalog in the original source. Their
# labels are Korean CP949, while item codes remain ASCII and can be parsed
# without translating names. Prices come from OpenItem/*.txt, never the client.
$shopRows = @()
foreach ($file in $files) {
    $npcId = $idsByFile[$file.Name.ToLowerInvariant()]
    $text = [Text.Encoding]::GetEncoding(949).GetString([IO.File]::ReadAllBytes($file.FullName))
    foreach ($line in ($text -split "`r?`n")) {
        if ($line.TrimStart().StartsWith('//')) { continue }
        $kind = 0
        if ($line -match '^\s*\*무기판매') { $kind = 1 }
        elseif ($line -match '^\s*\*방어구판매') { $kind = 2 }
        elseif ($line -match '^\s*\*잡화판매') { $kind = 3 }
        if ($kind -eq 0) { continue }
        foreach ($match in [regex]::Matches($line.ToUpperInvariant(), '\b[A-Z]{2}\d{3}\b')) {
            $code = $match.Value
            if ($itemPrices.ContainsKey($code)) {
                $buy = [Math]::Max(1, $itemPrices[$code])
                $shopRows += [pscustomobject]@{ NpcId = $npcId; Code = $code; Buy = $buy; Sell = [Math]::Max(1, [Math]::Floor($buy / 2)); Kind = $kind }
            }
        }
    }
}
$shopRows = @($shopRows | Sort-Object NpcId, Code -Unique)

# Rebuild the catalog using only ASCII item codes. The original section labels
# are CP949 and are not required to make the client's general shop tab work.
$shopRows = @()
foreach ($file in $files) {
    $npcId = $idsByFile[$file.Name.ToLowerInvariant()]
    $text = [Text.Encoding]::GetEncoding(949).GetString([IO.File]::ReadAllBytes($file.FullName))
    foreach ($line in ($text -split "`r?`n")) {
        if ($line.TrimStart().StartsWith('//')) { continue }
        foreach ($match in [regex]::Matches($line.ToUpperInvariant(), '\b[A-Z]{2}\d{3}\b')) {
            $code = $match.Value
            if (-not $itemPrices.ContainsKey($code)) { continue }
            $buy = [Math]::Max(1, $itemPrices[$code])
            $shopRows += [pscustomobject]@{ NpcId = $npcId; Code = $code; Buy = $buy; Sell = [Math]::Max(1, [Math]::Floor($buy / 2)); Kind = 3 }
        }
    }
}
$shopRows = @($shopRows | Sort-Object NpcId, Code -Unique)

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
if ($MapNpcCsv) {
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
                $rawX = [int64]$coordinates.X * $MapNpcCoordinateScale
                $rawY = [int64]$coordinates.Y * $MapNpcCoordinateScale
                $rawZ = [int64]$coordinates.Z * $MapNpcCoordinateScale
                if ($rawX -lt [int32]::MinValue -or $rawX -gt [int32]::MaxValue -or
                    $rawY -lt [int32]::MinValue -or $rawY -gt [int32]::MaxValue -or
                    $rawZ -lt [int32]::MinValue -or $rawZ -gt [int32]::MaxValue) {
                    throw "Coordenada fora do intervalo int32 para $legacyFile em $sourceMap"
                }
                $x = [int]$rawX; $y = [int]$rawY; $z = [int]$rawZ
                $angle = [int]$coordinates.AngleY
                $enabled = if ([int]$coordinates.Enabled -ne 0 -and $mapId -ge 0) { 1 } else { 0 }
            }
            $spawns += [pscustomobject]@{ Id = $id; MapId = $mapId; SourceMap = $sourceMap; X = $x; Y = $y; Z = $z; Angle = $angle; Enabled = $enabled }
        }
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
foreach ($npc in $definitions) {
    [void]$sql.AppendLine("IF NOT EXISTS (SELECT 1 FROM dbo.NpcShop WHERE NpcId=$($npc.Id)) INSERT INTO dbo.NpcShop (NpcId, ShopName, Enabled) VALUES ($($npc.Id), $(SqlString $npc.Name), 1);")
}
foreach ($shop in $shopRows) {
    $code = SqlString $shop.Code
    [void]$sql.AppendLine("IF EXISTS (SELECT 1 FROM dbo.ItemList l INNER JOIN dbo.NpcShop s ON s.NpcId=$($shop.NpcId) WHERE l.LegacyCode=$code AND s.Enabled=1) BEGIN IF EXISTS (SELECT 1 FROM dbo.NpcShopItem si INNER JOIN dbo.NpcShop s ON s.ShopId=si.ShopId WHERE s.NpcId=$($shop.NpcId) AND si.ItemCode=(SELECT TOP 1 ItemCode FROM dbo.ItemList WHERE LegacyCode=$code)) UPDATE dbo.NpcShopItem SET BuyPrice=$($shop.Buy), SellPrice=$($shop.Sell), RequiredLevel=(SELECT RequiredLevel FROM dbo.ItemList WHERE ItemCode=dbo.NpcShopItem.ItemCode), Enabled=1 WHERE ShopId=(SELECT ShopId FROM dbo.NpcShop WHERE NpcId=$($shop.NpcId)) AND ItemCode=(SELECT TOP 1 ItemCode FROM dbo.ItemList WHERE LegacyCode=$code) ELSE INSERT INTO dbo.NpcShopItem (ShopId, ItemCode, BuyPrice, SellPrice, Stock, RequiredLevel, Enabled) SELECT ShopId, ItemCode, $($shop.Buy), $($shop.Sell), NULL, RequiredLevel, 1 FROM dbo.NpcShop CROSS JOIN dbo.ItemList WHERE NpcId=$($shop.NpcId) AND LegacyCode=$code; END")
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
Write-Host "Catalogo de lojas importado: $($shopRows.Count) itens"
if (-not $MapNpcCsv) {
    Write-Warning 'Catalogo/lojas atualizados sem alterar NpcSpawn: forneca -MapNpcCsv exportado de MapNPC/Field.'
} elseif ($enabledCount -eq 0) {
    Write-Warning 'Nenhuma instancia foi habilitada: os backups fornecidos nao possuem coordenadas MapNPC. Forneca -MapNpcCsv para ativar spawns reais.'
}
if ($MapNpcCsv) {
    Write-Host "Escala de coordenadas MapNPC aplicada: $MapNpcCoordinateScale (Fortress usa X/Y/Z << 8; use 1 se o CSV ja estiver em ponto fixo)."
}
