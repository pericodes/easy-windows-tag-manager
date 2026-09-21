$ErrorActionPreference = "Stop"

$package = Get-AppxPackage -Name "MediaTags" -ErrorAction SilentlyContinue
if ($package) {
    $package | Remove-AppxPackage
    Write-Host "Paquete MediaTags desregistrado."
}
else {
    Write-Host "No hay ningún paquete MediaTags instalado."
}
