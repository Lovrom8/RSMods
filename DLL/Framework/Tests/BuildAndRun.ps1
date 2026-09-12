#!/usr/bin/env pwsh
# Builds and runs the Framework unit tests.
#
# Each *Tests.cpp is a standalone console program with its own main() that
# returns 0 on success and non-zero on failure. The framework is host-agnostic
# (no windows.h / stdafx), so each test compiles against only the handful of
# framework translation units it actually uses.
#
# Run locally:   pwsh DLL/Framework/Tests/BuildAndRun.ps1
# CI entrypoint: appveyor.yml (the `test_script` step runs this script on the develop branch).
#
# Exits non-zero if any test fails to build or reports test failures.

$ErrorActionPreference = 'Stop'

$TestsDir  = $PSScriptRoot
$Framework = Split-Path $TestsDir -Parent            # ...\DLL\Framework
$OutDir    = Join-Path $TestsDir 'bin'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# Extra framework translation units each test must link against (besides its own .cpp).
$Tests = @(
    @{ Name = 'ConflictResolverTests';  Sources = @() },  # header-only resolver
    @{ Name = 'ResourceLedgerTests';    Sources = @('ResourceLedger.cpp') },
    @{ Name = 'AspectRatioTests';      Sources = @() },  # header-only aspect math
    @{ Name = 'CommandRouterTests';     Sources = @('CommandRouter.cpp', 'CommandCollisionDiagnostics.cpp') },
    @{ Name = 'MainThreadInboxTests';   Sources = @('MainThreadInbox.cpp') },
    @{ Name = 'StateMachineTests';      Sources = @('ModRegistry.cpp', 'ResourceLedger.cpp', 'CommandRouter.cpp', 'MainThreadInbox.cpp', 'CommandCollisionDiagnostics.cpp') }
)

# Locate the MSVC developer environment (matches the DLL's v143 toolset).
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found; install Visual Studio with the 'Desktop development with C++' workload." }
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw "No Visual Studio install with the C++ toolset found." }
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars64.bat not found at $vcvars" }

$failed = @()
foreach ($t in $Tests) {
    $name = $t.Name
    $exe  = Join-Path $OutDir "$name.exe"
    $srcs = @((Join-Path $TestsDir "$name.cpp")) + ($t.Sources | ForEach-Object { Join-Path $Framework $_ })
    $srcArgs = ($srcs | ForEach-Object { "`"$_`"" }) -join ' '

    Write-Host "=== Building $name ===" -ForegroundColor Cyan
    # cd into the output dir so intermediate .obj files land there instead of the source tree.
    # Under Windows PowerShell 5.1 with ErrorActionPreference=Stop, any line a native exe
    # writes to stderr is turned into a terminating error, but the tests deliberately log to
    # stderr (LOG_ERROR) when exercising fault paths, so keep Stop scoped to cmdlets only.
    # vcvars is silenced on both streams (>nul 2>nul): on some machines it emits a stray
    # "vswhere.exe is not recognized" to stderr while still setting up the toolset correctly.
    # cl runs as a separate `&&` command, so its diagnostics are unaffected; a genuine vcvars
    # failure still surfaces as a missing `cl` and a non-zero exit below.
    $build = "call `"$vcvars`" >nul 2>nul && cd /d `"$OutDir`" && cl /nologo /std:c++20 /EHsc /W3 $srcArgs /Fe:`"$exe`""
    & { $ErrorActionPreference = 'Continue'; & cmd /c $build }
    if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED: $name" -ForegroundColor Red; $failed += "$name (build)"; continue }

    Write-Host "=== Running $name ===" -ForegroundColor Cyan
    & { $ErrorActionPreference = 'Continue'; & $exe }
    if ($LASTEXITCODE -ne 0) { Write-Host "TESTS FAILED: $name" -ForegroundColor Red; $failed += "$name (run)" }
}

Write-Host ""
if ($failed.Count -gt 0) {
    Write-Host "FRAMEWORK TESTS FAILED: $($failed -join ', ')" -ForegroundColor Red
    exit 1
}
Write-Host "ALL FRAMEWORK TESTS PASSED" -ForegroundColor Green
exit 0
