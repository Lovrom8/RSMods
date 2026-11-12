param([string]$OutputDirectory = (Join-Path $PSScriptRoot 'bin'))
$ErrorActionPreference = 'Stop'
$outputPath = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsPath 'VC/Auxiliary/Build/vcvars32.bat'
$source = Join-Path $PSScriptRoot 'MemoryProtectionTests.cpp'
$exe = Join-Path $outputPath 'MemoryProtectionTests.exe'
$detours = Join-Path $PSScriptRoot '../../../Lib/Detours/detours.lib'
& cmd /c "call `"$vcvars`" -vcvars_ver=14.29 >nul 2>nul && cl /nologo /std:c++latest /EHsc /W4 /WX /MT /O2 /Gy /GL /Fo:`"$outputPath/`" `"$source`" /Fe:`"$exe`" /link /OPT:REF /MANIFEST:EMBED `"$detours`" advapi32.lib"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
foreach ($mode in @('wine_get_version', 'wine_get_build_id', 'wine_get_host_version', 'HKCU', 'HKLM', 'native')) {
    & $exe $mode
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
foreach ($case in @(
    @('wine_get_host_version', 'auto', 'Linux'),
    @('wine_get_host_version', 'auto', 'Darwin'),
    @('wine_get_host_version', 'auto', 'unknown'),
    @('wine_get_host_version', 'auto', 'Linux', 'same'),
    @('wine_get_version', 'windows'),
    @('native', 'wine'),
    @('native', 'invalid')
)) {
    & $exe @case
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
& $exe
exit $LASTEXITCODE

