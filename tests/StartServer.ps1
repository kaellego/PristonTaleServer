[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [int]$Port = 5055,
    [int]$WaitSeconds = 5
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "x64\$Configuration"
$serverDirectory = Join-Path $projectRoot 'src\GameServer'
$serverPath = Join-Path $buildDirectory 'GameServer.exe'
$stdoutPath = Join-Path $serverDirectory 'GameServer.stdout.log'
$stderrPath = Join-Path $serverDirectory 'GameServer.stderr.log'

if (-not (Test-Path -LiteralPath $serverPath)) {
    throw "Executável não encontrado: $serverPath. Compile primeiro com .\build.ps1."
}

$running = Get-Process -Name GameServer -ErrorAction SilentlyContinue
if ($running) {
    Write-Host "GameServer já está em execução (PID $($running[0].Id))."
    exit 0
}

Remove-Item -LiteralPath $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue
$process = Start-Process -FilePath $serverPath `
    -WorkingDirectory $serverDirectory `
    -RedirectStandardOutput $stdoutPath `
    -RedirectStandardError $stderrPath `
    -PassThru

Write-Host "GameServer iniciado (PID $($process.Id))."
Start-Sleep -Seconds $WaitSeconds

$process.Refresh()
if ($process.HasExited) {
    Write-Host "O servidor encerrou com código $($process.ExitCode)." -ForegroundColor Red
    if (Test-Path -LiteralPath $stdoutPath) { Get-Content -LiteralPath $stdoutPath }
    if (Test-Path -LiteralPath $stderrPath) { Get-Content -LiteralPath $stderrPath }
    exit $process.ExitCode
}

$connection = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
if (-not $connection) {
    Write-Warning "Processo está ativo, mas a porta $Port ainda não está aceitando conexões."
    exit 2
}

Write-Host "Servidor aceitando conexões em 127.0.0.1:$Port." -ForegroundColor Green
Write-Host "Log: $stdoutPath"
