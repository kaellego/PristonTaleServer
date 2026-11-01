$ErrorActionPreference = 'Stop'
docker compose up -d sqlserver | Out-Host
Write-Host 'Aguardando o SQL Server ficar pronto...'
for ($i = 0; $i -lt 60; $i++) {
    docker exec pristontale-sqlserver /opt/mssql-tools18/bin/sqlcmd -S localhost -U sa -P 'PristonDev_2026!' -C -Q 'SELECT 1' 2>$null
    if ($LASTEXITCODE -eq 0) { break }
    Start-Sleep -Seconds 2
    if ($i -eq 59) { throw 'SQL Server nao ficou pronto dentro do tempo limite.' }
}
Get-Content -Raw (Join-Path $PSScriptRoot 'init.sql') | docker exec -i pristontale-sqlserver /opt/mssql-tools18/bin/sqlcmd -S localhost -U sa -P 'PristonDev_2026!' -C
if ($LASTEXITCODE -ne 0) { throw 'Falha ao aplicar o schema.' }
Write-Host 'Banco configurado com sucesso.'
