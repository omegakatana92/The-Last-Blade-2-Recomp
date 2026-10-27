# Configure and build the Windows x64 Release player.
# Usage: .\scripts\build-windows.ps1 [-RomDir .\roms] [-Package]
param(
    [string]$RomDir = "",
    [switch]$Package
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($RomDir)) { $RomDir = Join-Path $Root "roms" }
$RomDir = (Resolve-Path -LiteralPath $RomDir).Path
$Build = Join-Path $Root "build-windows"

$help = cmake --help 2>&1 | Out-String
if ($help -match "Visual Studio 18 2026") {
    $gen = @("-G", "Visual Studio 18 2026", "-A", "x64")
} elseif ($help -match "Visual Studio 17 2022") {
    $gen = @("-G", "Visual Studio 17 2022", "-A", "x64")
} else {
    $gen = @()
}

cmake -S $Root -B $Build @gen "-DLASTBLADE2_ROM_DIR=$RomDir"
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
cmake --build $Build --config Release --target lastblade2
if ($LASTEXITCODE -ne 0) { throw "build failed" }

if ($Package) { & (Join-Path $PSScriptRoot "package-windows.ps1") }
