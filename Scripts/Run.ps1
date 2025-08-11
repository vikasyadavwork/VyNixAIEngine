$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$gamePath = Join-Path $repoRoot 'out/build/vynix-release/bin/Release/Skybound.exe'
if (-not (Test-Path -LiteralPath $gamePath)) { throw 'Build the Release configuration first with Scripts/Build.ps1.' }
& $gamePath
