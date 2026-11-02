[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [int]$WaitSeconds = 5,
    [switch]$Restart,
    [string]$ItemSecret = $env:ZEROPT_ITEM_SECRET
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$serverDirectory = Join-Path $projectRoot "x64\$Configuration"
$serverPath = Join-Path $serverDirectory 'GameServer.exe'
$stdoutPath = Join-Path $serverDirectory 'GameServer.stdout.log'
$stderrPath = Join-Path $serverDirectory 'GameServer.stderr.log'
$ports = @(5055, 5056)

if (-not (Test-Path -LiteralPath $serverPath)) { throw "Executavel nao encontrado: $serverPath. Compile primeiro com .\build.ps1." }
if ([string]::IsNullOrWhiteSpace($ItemSecret) -or $ItemSecret.Length -lt 32) { $ItemSecret = 'local-development-secret-32-characters-min' }

$running = @(Get-CimInstance Win32_Process -Filter "Name='GameServer.exe'" | Where-Object { $_.ExecutablePath -and $_.ExecutablePath -ieq $serverPath })
if ($running.Count -gt 0 -and -not $Restart) {
    $existingPorts = @($ports | Where-Object { Test-NetConnection 127.0.0.1 -Port $_ -InformationLevel Quiet -WarningAction SilentlyContinue })
    if ($existingPorts.Count -eq $ports.Count) { Write-Host 'GameServer ja esta ativo e escutando as portas 5055 e 5056.'; exit 0 }
    throw 'Ja existe uma instancia do GameServer sem as duas portas ativas. Use -Restart.'
}
if ($running.Count -gt 0) { $running | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }; Start-Sleep -Seconds 1 }

Remove-Item -LiteralPath $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue
$legacyData = Join-Path $projectRoot 'src\GameServer\Data'
$runtimeData = Join-Path $serverDirectory 'Data'
foreach ($folder in @('Character', 'Inventory', 'Warehouse')) {
    $sourceFolder = Join-Path $legacyData $folder
    $targetFolder = Join-Path $runtimeData $folder
    if (Test-Path -LiteralPath $sourceFolder) {
        New-Item -ItemType Directory -Force -Path $targetFolder | Out-Null
        Get-ChildItem -LiteralPath $sourceFolder -File | ForEach-Object {
            $target = Join-Path $targetFolder $_.Name
            if (-not (Test-Path -LiteralPath $target)) {
                Copy-Item -LiteralPath $_.FullName -Destination $target
            }
        }
    }
}
$previousSecret = $env:ZEROPT_ITEM_SECRET
$env:ZEROPT_ITEM_SECRET = $ItemSecret
try {
    $process = Start-Process -FilePath $serverPath -WorkingDirectory $serverDirectory -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath -PassThru
} finally { $env:ZEROPT_ITEM_SECRET = $previousSecret }

Write-Host "GameServer iniciado (PID $($process.Id))."
Start-Sleep -Seconds $WaitSeconds
$process.Refresh()
if ($process.HasExited) {
    Write-Host "O servidor encerrou com codigo $($process.ExitCode)." -ForegroundColor Red
    if (Test-Path -LiteralPath $stdoutPath) { Get-Content -LiteralPath $stdoutPath }
    if (Test-Path -LiteralPath $stderrPath) { Get-Content -LiteralPath $stderrPath }
    exit $process.ExitCode
}

$listening = @($ports | Where-Object { Test-NetConnection 127.0.0.1 -Port $_ -InformationLevel Quiet -WarningAction SilentlyContinue })
if ($listening.Count -lt $ports.Count) {
    Write-Warning 'O processo esta ativo, mas as portas 5055 e 5056 nao estao aceitando conexoes.'
    if (Test-Path -LiteralPath $stdoutPath) { Get-Content -LiteralPath $stdoutPath -Tail 40 }
    if (Test-Path -LiteralPath $stderrPath) { Get-Content -LiteralPath $stderrPath -Tail 40 }
    exit 2
}
Write-Host 'Servidor aceitando conexoes em 127.0.0.1:5055 e 127.0.0.1:5056.' -ForegroundColor Green
Write-Host "Log: $stdoutPath"
