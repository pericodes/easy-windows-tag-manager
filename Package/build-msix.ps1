param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("x64")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$PackageDir = $PSScriptRoot
$RepoRoot = Split-Path $PackageDir -Parent
$OutDir = Join-Path $RepoRoot "out\$Platform\$Configuration"
$StagingDir = Join-Path $PackageDir "obj\sparse"
$MsixPath = Join-Path $OutDir "MediaTags.msix"
$CertPath = Join-Path $OutDir "MediaTags.cer"

function Find-SdkTool([string]$Name) {
    $candidates = @()
    $kitRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10"
    $kitBin = Join-Path $kitRoot "bin"

    if (Test-Path $kitBin) {
        $candidates += Get-ChildItem -Path $kitBin -Directory -ErrorAction SilentlyContinue |
            ForEach-Object { Join-Path $_.FullName "x64\$Name" }
        $candidates += Join-Path $kitBin "x64\$Name"
    }

    $candidates += Join-Path $kitRoot "App Certification Kit\$Name"

    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) {
        $candidates += $cmd.Source
    }

    $tool = $candidates |
        Where-Object { $_ -and (Test-Path $_) } |
        Select-Object -First 1

    if (-not $tool) {
        throw "No se encontró $Name. SDK bin=$kitBin (existe=$(Test-Path $kitBin))."
    }

    Write-Host "Usando $Name : $tool"
    return $tool
}

$exe = Join-Path $OutDir "MediaTags.exe"
$dll = Join-Path $OutDir "MediaTagsShell.dll"

if (-not (Test-Path $exe) -or -not (Test-Path $dll)) {
    throw "No están los binarios en $OutDir."
}

New-Item -ItemType Directory -Force -Path (Join-Path $PackageDir "obj") | Out-Null
if (Test-Path $StagingDir) {
    Remove-Item $StagingDir -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $StagingDir | Out-Null
Copy-Item (Join-Path $PackageDir "AppxManifest.xml") (Join-Path $StagingDir "AppxManifest.xml")
Copy-Item (Join-Path $PackageDir "Assets") (Join-Path $StagingDir "Assets") -Recurse

$cert = @(Get-ChildItem Cert:\CurrentUser\My -ErrorAction SilentlyContinue |
    Where-Object { $_.Subject -eq "CN=MediaTags" }) |
    Select-Object -First 1

if (-not $cert) {
    try {
        $cert = New-SelfSignedCertificate `
            -Type Custom `
            -Subject "CN=MediaTags" `
            -KeyUsage DigitalSignature `
            -KeySpec Signature `
            -KeyExportPolicy Exportable `
            -HashAlgorithm SHA256 `
            -FriendlyName "MediaTags" `
            -CertStoreLocation "Cert:\CurrentUser\My" `
            -TextExtension @(
                "2.5.29.37={text}1.3.6.1.5.5.7.3.3",
                "2.5.29.19={text}"
            )
    } catch {
        throw "No se pudo crear el certificado CN=MediaTags: $($_.Exception.Message)"
    }
}

if (Test-Path $CertPath) {
    Remove-Item $CertPath -Force
}

Export-Certificate -Cert $cert -FilePath $CertPath | Out-Null

$makeAppx = Find-SdkTool "makeappx.exe"
$signTool = Find-SdkTool "signtool.exe"

if (Test-Path $MsixPath) {
    Remove-Item $MsixPath -Force
}

& $makeAppx pack /d $StagingDir /p $MsixPath /nv
if ($LASTEXITCODE -ne 0) {
    throw "MakeAppx falló con código $LASTEXITCODE"
}

& $signTool sign /fd SHA256 /sha1 $cert.Thumbprint $MsixPath
if ($LASTEXITCODE -ne 0) {
    throw "SignTool falló con código $LASTEXITCODE"
}

Write-Host "Paquete listo: $MsixPath"
