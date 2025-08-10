param([ValidateSet('Release','Debug')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path -Parent $PSScriptRoot)
try {
    cmake --preset vynix-windows
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    cmake --build out/build/vynix-release --config $Configuration --parallel 4
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
    ctest --test-dir out/build/vynix-release -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
} finally { Pop-Location }
