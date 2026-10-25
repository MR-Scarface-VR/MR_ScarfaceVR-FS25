param([string]$Path = (Join-Path $PSScriptRoot '..\build\dinput8.dll'))
$ErrorActionPreference = 'Stop'
$version = (Get-Item -LiteralPath $Path).VersionInfo
if ($version.ProductName -ne 'MR_ScarfaceVR for FS25' -or
    $version.CompanyName -ne 'MR_Scarface' -or
    $version.ProductVersion -ne '0.1.0.4' -or
    $version.FileVersion -ne '0.1.0.4' -or
    $version.OriginalFilename -ne 'dinput8.dll') {
    throw 'DLL version metadata does not match the SignPath artifact restrictions.'
}
Write-Output 'PASS: DLL product, versions and original filename match SignPath restrictions.'
