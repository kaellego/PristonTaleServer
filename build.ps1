[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('x64', 'x86')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
if (-not (Test-Path $msbuild)) { throw "MSBuild não encontrado: $msbuild" }

$protobufHeader = Join-Path $PSScriptRoot 'vcpkg-installed\x64-windows\include\google\protobuf\message.h'
if (-not (Test-Path $protobufHeader)) {
    throw 'Dependencia Protobuf ausente. Execute: C:\vcpkg\vcpkg.exe install --x-install-root=vcpkg-installed --triplet x64-windows'
}

& $msbuild (Join-Path $PSScriptRoot 'PristonTaleServer.sln') /m:1 /p:Configuration=$Configuration /p:Platform=$Platform /v:minimal
if ($LASTEXITCODE -ne 0) { throw "Falha na compilação. Código: $LASTEXITCODE" }
