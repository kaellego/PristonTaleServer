$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$cargo = Get-Command cargo -ErrorAction SilentlyContinue
if (-not $cargo) {
    $cargoPath = Join-Path $env:USERPROFILE '.cargo\bin\cargo.exe'
    if (-not (Test-Path -LiteralPath $cargoPath)) {
        throw 'Cargo nao encontrado. Instale Rustup/Rust e tente novamente.'
    }
    $cargo = $cargoPath
} else {
    $cargo = $cargo.Source
}

& $cargo build --release --locked
if ($LASTEXITCODE -ne 0) {
    throw "Falha na compilacao. Codigo: $LASTEXITCODE"
}

$binary = Join-Path $repoRoot 'target\release\rustpt-server.exe'
if (-not (Test-Path -LiteralPath $binary)) {
    throw "Executavel nao encontrado: $binary"
}

Write-Host "Servidor compilado: $binary"
