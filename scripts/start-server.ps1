$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot
$binary = Join-Path $repoRoot 'target\release\rustpt-server.exe'

if (-not (Test-Path -LiteralPath $binary)) {
    & (Join-Path $PSScriptRoot 'build-server.ps1')
}

if ([string]::IsNullOrWhiteSpace($env:ZEROPT_SESSION_SECRET)) {
    throw 'Defina ZEROPT_SESSION_SECRET antes de iniciar o servidor.'
}
if ($env:ZEROPT_SESSION_SECRET.Length -lt 32) {
    throw 'ZEROPT_SESSION_SECRET deve possuir pelo menos 32 caracteres.'
}

& $binary
exit $LASTEXITCODE
