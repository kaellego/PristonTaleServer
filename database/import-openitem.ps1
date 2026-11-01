[CmdletBinding()]
param(
    [string]$OpenItemPath = 'C:\Users\weeel\Desktop\ZeroPT-Server\GameServer\OpenItem',
    [string]$LegacySourcePath = 'C:\Users\weeel\Desktop\_OLD\ZeroPT-Source\Source ZeroPT',
    [string]$Container = 'pristontale-sqlserver',
    [string]$Password = 'PristonDev_2026!'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path $OpenItemPath)) { throw "Diretório de itens não encontrado: $OpenItemPath" }

function SqlString([string]$value) {
    if ($null -eq $value) { return "NULL" }
    return "N'" + ($value.Replace("'", "''")) + "'"
}

function Get-ItemMetadata([System.IO.FileInfo]$file) {
    $lines = Get-Content -LiteralPath $file.FullName -Encoding Default
    $code = $file.BaseName.Split('_')[-1]
    $nameMatch = $lines | Select-String '^\*Name\s*"([^"]*)"' | Select-Object -First 1
    $name = if ($nameMatch) { $nameMatch.Matches[0].Groups[1].Value } else { $code }

    $level = 1
    # O catálogo usa a codificação coreana legada; a seção de requisitos começa
    # após os atributos de combate, portanto procuramos o primeiro valor simples
    # nessa faixa, que corresponde ao nível mínimo.
    for ($i = 45; $i -lt [Math]::Min($lines.Count, 80); $i++) {
        if ($lines[$i] -match '^\*[^*]*\s+(\d+)\s*$') { $level = [int]$Matches[1]; break }
    }

    $jobMask = [uint64]4294967295
    $jobBits = @{ Mechanician = 1; Fighter = 2; Pikeman = 4; Archer = 8; Knight = 65536; Atalanta = 131072; Priest = 262144; Magician = 524288; Assassin = 1048576; Shaman = 2097152 }
    $jobs = @($lines | Select-String '\*\*.*(Mechanician|Fighter|Pikeman|Archer|Knight|Atalanta|Priest|Magician|Assassin|Shaman)' | ForEach-Object { foreach ($job in $jobBits.Keys) { if ($_.Line -match $job) { $jobBits[$job] } } } | Select-Object -Unique)
    if ($jobs.Count -gt 0) { $jobMask = [uint64]0; foreach ($bit in $jobs) { $jobMask = $jobMask -bor [uint64]$bit } }

    $slot = -1
    if ($code -match '^(2_)?(U)?W') { $slot = 0 }
    $consumable = if ($code -match '^(PL|PM|PS|P)') { 1 } else { 0 }
    [pscustomobject]@{ Code = $code; Name = $name; Level = $level; Slot = $slot; JobMask = $jobMask; Consumable = $consumable }
}

$sinItem = Get-ChildItem -LiteralPath $LegacySourcePath -Recurse -Filter 'sinItem.h' | Select-Object -First 1
if (-not $sinItem) { throw "sinItem.h não encontrado em $LegacySourcePath" }
$constants = @{}
foreach ($line in Get-Content $sinItem.FullName -Encoding Default) {
    if ($line -match '^\s*#define\s+(sin[A-Za-z0-9_]+)\s+0x([0-9A-Fa-f]+)') { $constants[$Matches[1]] = [Convert]::ToInt64($Matches[2], 16) }
}
function Resolve-ItemCode([string]$legacyCode) {
    if ($legacyCode -notmatch '^[A-Za-z]{2}\d{3}$') { return $null }
    $group = 'sin' + $legacyCode.Substring(0, 2) + '1'
    $variant = 'sin' + $legacyCode.Substring($legacyCode.Length - 2)
    if (!$constants.ContainsKey($group) -or !$constants.ContainsKey($variant)) { return $null }
    return [int]($constants[$group] -bor $constants[$variant])
}

$items = Get-ChildItem -LiteralPath $OpenItemPath -File -Filter '*.txt' | ForEach-Object { $item = Get-ItemMetadata $_; $item | Add-Member NoteProperty NumericCode (Resolve-ItemCode $item.Code) -PassThru } | Where-Object { $null -ne $_.NumericCode } | Group-Object NumericCode | ForEach-Object { $_.Group | Select-Object -First 1 }
if ($items.Count -eq 0) { throw 'Nenhuma definição de item foi encontrada.' }

$sql = [System.Text.StringBuilder]::new()
[void]$sql.AppendLine('USE GameDB;')
[void]$sql.AppendLine('SET XACT_ABORT ON; BEGIN TRANSACTION;')
foreach ($item in $items) {
    [void]$sql.AppendLine("IF EXISTS (SELECT 1 FROM dbo.ItemList WHERE ItemCode=$($item.NumericCode)) UPDATE dbo.ItemList SET LegacyCode=$(SqlString $item.Code), ItemName=$(SqlString $item.Name), RequiredLevel=$($item.Level), EquipmentSlot=$($item.Slot), JobCodeMask=$($item.JobMask), Consumable=$($item.Consumable) WHERE ItemCode=$($item.NumericCode) ELSE INSERT INTO dbo.ItemList (ItemCode, LegacyCode, ItemName, RequiredLevel, EquipmentSlot, JobCodeMask, Consumable) VALUES ($($item.NumericCode), $(SqlString $item.Code), $(SqlString $item.Name), $($item.Level), $($item.Slot), $($item.JobMask), $($item.Consumable));")
}
[void]$sql.AppendLine('COMMIT TRANSACTION;')
$temporarySql = Join-Path ([System.IO.Path]::GetTempPath()) 'pristontale-openitem-import.sql'
[System.IO.File]::WriteAllText($temporarySql, $sql.ToString(), [System.Text.UTF8Encoding]::new($false))
try {
    Get-Content -Raw -LiteralPath $temporarySql | docker exec -i $Container /opt/mssql-tools18/bin/sqlcmd -S localhost -U sa -P $Password -C -b
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao importar o catálogo de itens.' }
} finally { Remove-Item -LiteralPath $temporarySql -Force -ErrorAction SilentlyContinue }
Write-Host "Itens importados: $($items.Count)"
