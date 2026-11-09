#!/usr/bin/env pwsh
# Builds and runs the Win32 MSVC tests for x86 hook thunks.
param([string]$OutputDirectory = (Join-Path $PSScriptRoot 'bin'))
$ErrorActionPreference = 'Stop'
$outputPath = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = & $vswhere -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw "No Visual Studio install with the C++ toolset found." }
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars32.bat'
$source = Join-Path $PSScriptRoot 'EnumerationPackageTests.cpp'
$exe = Join-Path $outputPath 'EnumerationPackageTests.exe'
$build = "call `"$vcvars`" >nul 2>nul && cd /d `"$outputPath`" && cl /nologo /std:c++latest /EHsc /W4 /WX /MDd /D_DEBUG `"$source`" /Fe:`"$exe`""
& { $ErrorActionPreference = 'Continue'; & cmd /c $build }
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& { $ErrorActionPreference = 'Continue'; & $exe }
exit $LASTEXITCODE
