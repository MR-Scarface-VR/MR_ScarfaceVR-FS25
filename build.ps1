param([ValidateSet('all', 'smoke', 'runtime', 'gpu', 'capture')][string]$Target = 'all')
$ErrorActionPreference = 'Stop'
$vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsRoot) { throw 'Existing MSVC x64 tools not found. No tools will be installed.' }
$vcVars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars64.bat'
$setupCommand = '"' + $vcVars + '" >nul && set'
& $env:ComSpec /d /s /c $setupCommand | ForEach-Object {
    if ($_ -match '^(PATH|INCLUDE|LIB|LIBPATH)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
if ($LASTEXITCODE -ne 0) { throw 'Existing MSVC environment initialization failed.' }
$buildDirectory = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
Push-Location -LiteralPath $buildDirectory
try {
    & cl.exe /nologo /std:c++20 /EHsc /W4 /WX /MT /O2 (Join-Path $PSScriptRoot 'tests\native_core_tests.cpp') /Fe:native_core_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Native core test compilation failed.' }
    & .\native_core_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Native core tests failed.' }
    & cl.exe /nologo /std:c++20 /EHsc /W4 /WX /MT /O2 (Join-Path $PSScriptRoot 'tests\openxr_flow_tests.cpp') /Fe:openxr_flow_tests.exe /link advapi32.lib
    if ($LASTEXITCODE -ne 0) { throw 'OpenXR flow test compilation failed.' }
    & .\openxr_flow_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'OpenXR flow tests failed.' }
    & cl.exe /nologo /std:c++20 /EHsc /W4 /WX /MT /O2 (Join-Path $PSScriptRoot 'tests\proxy_smoke.cpp') /Fe:proxy_smoke.exe /link dxguid.lib
    if ($LASTEXITCODE -ne 0) { throw 'Smoke test compilation failed.' }
    if ($Target -eq 'runtime') {
        & cl.exe /nologo /std:c++20 /EHsc /W4 /WX /MT /O2 (Join-Path $PSScriptRoot 'tests\runtime_probe.cpp') (Join-Path $PSScriptRoot 'native\openxr_bridge.cpp') /Fe:runtime_probe.exe /link d3d12.lib dxgi.lib advapi32.lib
        if ($LASTEXITCODE -ne 0) { throw 'Runtime probe compilation failed.' }
        & .\runtime_probe.exe
        if ($LASTEXITCODE -ne 0) { throw 'Runtime probe failed.' }
    }
    if ($Target -eq 'gpu') {
        & cl.exe /nologo /std:c++20 /EHsc /W4 /WX /MT /O2 (Join-Path $PSScriptRoot 'tests\gpu_copy_tests.cpp') /Fe:gpu_copy_tests.exe /link d3d12.lib dxgi.lib
        if ($LASTEXITCODE -ne 0) { throw 'GPU copy test compilation failed.' }
        & .\gpu_copy_tests.exe
        if ($LASTEXITCODE -ne 0) { throw 'GPU copy test failed.' }
    }
    if ($Target -in @('all', 'capture')) {
        $defines = @()
        if ($Target -eq 'capture') { $defines = @('/DFS25VR_DESKTOP_CAPTURE') }
        & rc.exe /nologo /fo version.res (Join-Path $PSScriptRoot 'native\version.rc')
        if ($LASTEXITCODE -ne 0) { throw 'Native DLL resource compilation failed.' }
        & cl.exe /nologo /std:c++20 /EHsc /W4 /WX /MT /O2 /LD @defines (Join-Path $PSScriptRoot 'native\proxy.cpp') (Join-Path $PSScriptRoot 'native\engine_probe.cpp') (Join-Path $PSScriptRoot 'native\openxr_bridge.cpp') version.res /link ('/DEF:' + (Join-Path $PSScriptRoot 'native\dinput8.def')) /OUT:dinput8.dll bcrypt.lib user32.lib advapi32.lib
        if ($LASTEXITCODE -ne 0) { throw 'Native proxy compilation failed.' }
    }
    & (Join-Path $PSScriptRoot 'tests\version_info_tests.ps1') -Path (Join-Path $buildDirectory 'dinput8.dll')
    & .\proxy_smoke.exe
    if ($LASTEXITCODE -ne 0) { throw 'Native proxy smoke test failed.' }
} finally {
    Pop-Location
}
