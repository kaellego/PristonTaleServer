param([string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root 'extern/soci'
$build = Join-Path $root 'build/soci'
$install = Join-Path $root 'extern/soci-install'

if (-not (Test-Path (Join-Path $source '.git'))) {
    git clone --depth 1 --branch v4.1.2 https://github.com/SOCI/soci.git $source
}

cmake -S $source -B $build -DSOCI_TESTS=OFF -DSOCI_ODBC=ON -DSOCI_SQLITE3=OFF -DSOCI_MYSQL=OFF -DSOCI_POSTGRESQL=OFF -DSOCI_ORACLE=OFF -DSOCI_FIREBIRD=OFF -DSOCI_DB2=OFF "-DCMAKE_INSTALL_PREFIX=$install"
cmake --build $build --config $Configuration --target install

$output = Join-Path $root "x64/$Configuration"
New-Item -ItemType Directory -Path $output -Force | Out-Null
foreach ($dll in @('soci_core_4_1.dll', 'soci_odbc_4_1.dll')) {
    Copy-Item (Join-Path $install "bin/$dll") $output -Force
}
