# Stage the Windows x64 distribution in dist/windows/.
# Usage: .\scripts\package-windows.ps1 [-Configuration Release]
param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root "build-windows"
$Exe = Join-Path $Build "$Configuration\Last blade 2.exe"
$Out = Join-Path $Root "dist\windows\Last Blade 2-Windows-x64"

if (-not (Test-Path -LiteralPath $Exe)) {
    throw "Missing $Exe. Build the Release target first (see README.md)."
}

Write-Host "Staging $Out"
if (Test-Path -LiteralPath $Out) { Remove-Item -LiteralPath $Out -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Out, "$Out\roms", "$Out\docs", "$Out\LICENSES" | Out-Null

Copy-Item -LiteralPath $Exe -Destination $Out
Copy-Item -LiteralPath (Join-Path $Build "$Configuration\SDL2.dll") -Destination $Out
Copy-Item -LiteralPath (Join-Path $Root "packaging\windows\README.txt") -Destination (Join-Path $Out "README.txt")
Copy-Item -LiteralPath (Join-Path $Root "packaging\windows\roms-README.txt") -Destination (Join-Path $Out "roms\README.txt")
Copy-Item -LiteralPath (Join-Path $Root "LICENSE") -Destination $Out
Copy-Item -LiteralPath (Join-Path $Root "THIRD_PARTY_NOTICES.md") -Destination $Out
Copy-Item -LiteralPath (Join-Path $Root "LICENSE-neogeorecomp") -Destination $Out
Copy-Item -LiteralPath (Join-Path $Root "NOTICE-neogeorecomp") -Destination $Out
Copy-Item -Path (Join-Path $Root "LICENSES\*") -Destination (Join-Path $Out "LICENSES")
Copy-Item -LiteralPath (Join-Path $Root "docs\game-notes.md") -Destination (Join-Path $Out "docs")
Copy-Item -LiteralPath (Join-Path $Root "docs\compatibility.md") -Destination (Join-Path $Out "docs")

$sums = @()
foreach ($name in @("Last blade 2.exe", "SDL2.dll")) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $Out $name) -Algorithm SHA256).Hash.ToLower()
    $sums += "$hash  $name"
}
$sums | Set-Content -LiteralPath (Join-Path $Out "SHA256SUMS.txt") -Encoding ascii

Write-Host "Wrote:"
Get-ChildItem -LiteralPath $Out -Recurse -File | ForEach-Object { "  " + $_.FullName.Substring($Out.Length + 1) }
