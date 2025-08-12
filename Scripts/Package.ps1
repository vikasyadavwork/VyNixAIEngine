$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'Build.ps1') -Configuration Release
Push-Location (Split-Path -Parent $PSScriptRoot)
try {
    cpack --config out/build/vynix-release/CPackConfig.cmake -C Release -B out/releases
    if ($LASTEXITCODE -ne 0) { throw 'Packaging failed.' }
} finally { Pop-Location }
