param([string]$OutputDirectory = (Join-Path $PSScriptRoot '../build'))
$ErrorActionPreference = 'Stop'
$origin = Get-Content -LiteralPath (Join-Path $PSScriptRoot '../third_party/openxr-loader/ORIGIN.json') -Raw | ConvertFrom-Json
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$dll = Join-Path $OutputDirectory 'openxr_loader.dll'
if ((Test-Path -LiteralPath $dll) -and (Get-FileHash -LiteralPath $dll).Hash -eq $origin.dllSha256) { return }
$package = Join-Path $OutputDirectory 'OpenXR.Loader.1.1.65.nupkg'
if (!(Test-Path -LiteralPath $package) -or (Get-FileHash -LiteralPath $package).Hash -ne $origin.packageSha256) {
    Invoke-WebRequest -Uri $origin.downloadUrl -OutFile $package
}
if ((Get-FileHash -LiteralPath $package).Hash -ne $origin.packageSha256) { throw 'Official OpenXR package checksum mismatch.' }
$archive = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $package).Path)
try {
    $entry = $archive.GetEntry('native/x64/release/bin/openxr_loader.dll')
    if (!$entry) { throw 'Official x64 loader entry missing.' }
    $inputStream = $entry.Open()
    $outputStream = [IO.File]::Create($dll + '.new')
    try { $inputStream.CopyTo($outputStream) } finally { $outputStream.Dispose(); $inputStream.Dispose() }
} finally { $archive.Dispose() }
if ((Get-FileHash -LiteralPath ($dll + '.new')).Hash -ne $origin.dllSha256) { throw 'Official OpenXR DLL checksum mismatch.' }
Move-Item -LiteralPath ($dll + '.new') -Destination $dll -Force
Write-Output 'PASS: pinned official Khronos x64 OpenXR loader downloaded and verified.'
