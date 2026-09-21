param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("x64")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$OutDir = Join-Path (Split-Path $PSScriptRoot -Parent) "out\$Platform\$Configuration"
$exe = Join-Path $OutDir "MediaTags.exe"

if (-not (Test-Path $exe)) {
    throw "Compila MediaTags.slnx en $Platform | $Configuration primero."
}

& $exe --install
