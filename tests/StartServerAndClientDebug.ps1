[CmdletBinding()]
param(
    [int]$CaptureSeconds = 60
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$serverDir = Join-Path $root 'src\GameServer'
$serverExe = Join-Path $root 'x64\Debug\GameServer.exe'
$clientDir = 'C:\Users\weeel\Desktop\ZeroPT-Client'
$clientExe = Join-Path $clientDir 'Game.exe'
$report = Join-Path $root 'tests\connection-debug.txt'

if (-not (Test-Path -LiteralPath $serverExe)) { throw "Servidor não encontrado: $serverExe" }
if (-not (Test-Path -LiteralPath $clientExe)) { throw "Cliente não encontrado: $clientExe" }
if (-not (Test-Path -LiteralPath (Join-Path $clientDir 'Settings.ini'))) { throw "Settings.ini não encontrado em $clientDir" }

Get-Process GameServer,Game -ErrorAction SilentlyContinue | Stop-Process -Force
Remove-Item -LiteralPath $report -Force -ErrorAction SilentlyContinue

$serverOut = Join-Path $serverDir 'GameServer.stdout.log'
$serverErr = Join-Path $serverDir 'GameServer.stderr.log'
$server = Start-Process $serverExe -WorkingDirectory $serverDir -RedirectStandardOutput $serverOut -RedirectStandardError $serverErr -PassThru
Start-Sleep -Seconds 5

$client = Start-Process $clientExe -WorkingDirectory $clientDir -WindowStyle Normal -PassThru
for ($i = 0; $i -lt 20 -and $client.MainWindowHandle -eq 0; $i++) {
    Start-Sleep -Milliseconds 250
    $client.Refresh()
}
if ($client.MainWindowHandle -ne 0) {
    if (-not ('ClientWindow' -as [type])) {
        Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class ClientWindow {
    [DllImport("user32.dll")] public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@
    [ClientWindow]::ShowWindowAsync($client.MainWindowHandle, 9) | Out-Null
    [ClientWindow]::SetForegroundWindow($client.MainWindowHandle) | Out-Null
}
}
$started = Get-Date

"Inicio: $started" | Set-Content -LiteralPath $report -Encoding UTF8
"Servidor PID: $($server.Id) | Cliente PID: $($client.Id)" | Add-Content -LiteralPath $report
"Cliente: $clientExe" | Add-Content -LiteralPath $report
"Configuração: $clientDir\Settings.ini" | Add-Content -LiteralPath $report
"" | Add-Content -LiteralPath $report

Write-Host "Servidor PID $($server.Id) e cliente PID $($client.Id) iniciados."
Write-Host "Clique em Entrar no cliente. Capturando diagnóstico por $CaptureSeconds segundos..."

for ($i = 0; $i -lt $CaptureSeconds; $i++) {
    $server.Refresh(); $client.Refresh()
    $connections = Get-NetTCPConnection -OwningProcess $client.Id -ErrorAction SilentlyContinue |
        Where-Object { $_.RemotePort -in 5055,10009 }
    $stamp = Get-Date -Format 'HH:mm:ss'
    if ($connections) {
        $line = "$stamp TCP: " + (($connections | ForEach-Object { "$($_.RemoteAddress):$($_.RemotePort) [$($_.State)]" }) -join ', ')
        Write-Host $line -ForegroundColor Green
        Add-Content -LiteralPath $report $line
    }
    Start-Sleep -Seconds 1
}

"" | Add-Content -LiteralPath $report
'--- SERVER LOG ---' | Add-Content -LiteralPath $report
if (Test-Path $serverOut) { Get-Content $serverOut -Tail 100 | Add-Content -LiteralPath $report }
'--- SERVER STDERR ---' | Add-Content -LiteralPath $report
if (Test-Path $serverErr) { Get-Content $serverErr -Tail 100 | Add-Content -LiteralPath $report }

Write-Host "Diagnóstico salvo em: $report" -ForegroundColor Cyan
Read-Host 'Pressione ENTER para finalizar'
