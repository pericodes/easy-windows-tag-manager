param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("x64")]
    [string]$Platform = "x64",

    [Parameter(Mandatory = $true)]
    [string]$Stub
)

$ErrorActionPreference = "Stop"

$PackageDir = $PSScriptRoot
$RepoRoot = Split-Path $PackageDir -Parent
$OutDir = Join-Path $RepoRoot "out\$Platform\$Configuration"
$PayloadDir = Join-Path $PackageDir "obj\payload"
$ZipPath = Join-Path $PackageDir "obj\payload.zip"
$InstallerPath = Join-Path $OutDir "MediaTagsSetup.exe"

& (Join-Path $PackageDir "build-msix.ps1") -Configuration $Configuration -Platform $Platform

$required = @(
    "MediaTags.exe",
    "MediaTagsShell.dll",
    "MediaTags.msix",
    "MediaTags.cer"
)

foreach ($name in $required) {
    $path = Join-Path $OutDir $name
    if (-not (Test-Path $path)) {
        throw "Falta $path"
    }
}

if (Test-Path $PayloadDir) {
    Remove-Item $PayloadDir -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $PayloadDir | Out-Null

foreach ($name in $required) {
    Copy-Item (Join-Path $OutDir $name) (Join-Path $PayloadDir $name)
}

$loader = Join-Path $OutDir "WebView2Loader.dll"
if (Test-Path $loader) {
    Copy-Item $loader (Join-Path $PayloadDir "WebView2Loader.dll")
}

Copy-Item (Join-Path $OutDir "Web") (Join-Path $PayloadDir "Web") -Recurse

$webIndex = Join-Path $PayloadDir "Web\index.html"
if (-not (Test-Path $webIndex)) {
    throw "Falta Web\index.html en el payload del instalador"
}

if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
}

Compress-Archive -Path (Join-Path $PayloadDir "*") -DestinationPath $ZipPath -Force

$stubBytes = [IO.File]::ReadAllBytes((Resolve-Path $Stub))
$zipBytes = [IO.File]::ReadAllBytes($ZipPath)

$zipOffset = $stubBytes.Length
$zipSize = $zipBytes.Length

$footer = New-Object byte[] 16
[BitConverter]::GetBytes([uint32]0x4B50544D).CopyTo($footer, 0)
[BitConverter]::GetBytes([uint32]$zipOffset).CopyTo($footer, 4)
[BitConverter]::GetBytes([uint32]$zipSize).CopyTo($footer, 8)
[BitConverter]::GetBytes([uint32]1).CopyTo($footer, 12)

$output = New-Object byte[] ($stubBytes.Length + $zipBytes.Length + $footer.Length)
[Array]::Copy($stubBytes, 0, $output, 0, $stubBytes.Length)
[Array]::Copy($zipBytes, 0, $output, $stubBytes.Length, $zipBytes.Length)
[Array]::Copy($footer, 0, $output, $stubBytes.Length + $zipBytes.Length, $footer.Length)

[IO.File]::WriteAllBytes($InstallerPath, $output)

Write-Host "Instalador: $InstallerPath"
