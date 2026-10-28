<#
.SYNOPSIS
    Clones/updates the Rocksmith2014.NET (DLCB) repo, applies local patches, builds, and copies DLLs to Deps/.

.DESCRIPTION
    This script maintains a local checkout of iminashi's Rocksmith2014.NET library, applies
    any patches from the patches/ directory, builds all library projects, and copies the
    resulting DLLs into the Deps/ folder used by CFSM2.

    Run this whenever you want to update to the latest upstream DLCB or after modifying patches.

.EXAMPLE
    .\build-deps.ps1
    .\build-deps.ps1 -Branch main
    .\build-deps.ps1 -Clean
#>

param(
    [string]$Branch = "main",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$DepsDir = Split-Path -Parent $ScriptDir
$RepoDir = Join-Path $DepsDir "_rocksmith2014-net"
$PatchDir = Join-Path $ScriptDir "patches"
$RepoUrl = "https://github.com/iminashi/Rocksmith2014.NET.git"

# Library projects to build (in dependency order) and their output DLL names
$Projects = @(
    @{ Path = "src/Rocksmith2014.FSharpExtensions/Rocksmith2014.FSharpExtensions.fsproj"; Dll = "Rocksmith2014.FSharpExtensions.dll" }
    @{ Path = "src/Rocksmith2014.Common/Rocksmith2014.Common.fsproj";                     Dll = "Rocksmith2014.Common.dll" }
    @{ Path = "src/Rocksmith2014.XML/Rocksmith2014.XML.csproj";                            Dll = "Rocksmith2014.XML.dll" }
    @{ Path = "src/Rocksmith2014.XML.Extension/Rocksmith2014.XML.Extension.fsproj";       Dll = "Rocksmith2014.XML.Extension.dll" }
    @{ Path = "src/Rocksmith2014.XML.Processing/Rocksmith2014.XML.Processing.fsproj";     Dll = "Rocksmith2014.XML.Processing.dll" }
    @{ Path = "src/Rocksmith2014.SNG/Rocksmith2014.SNG.fsproj";                           Dll = "Rocksmith2014.SNG.dll" }
    @{ Path = "src/Rocksmith2014.Audio/Rocksmith2014.Audio.fsproj";                       Dll = "Rocksmith2014.Audio.dll" }
    @{ Path = "src/Rocksmith2014.DLCProject/Rocksmith2014.DLCProject.fsproj";             Dll = "Rocksmith2014.DLCProject.dll" }
    @{ Path = "src/Rocksmith2014.PSARC/Rocksmith2014.PSARC.fsproj";                       Dll = "Rocksmith2014.PSARC.dll" }
    @{ Path = "src/Rocksmith2014.Conversion/Rocksmith2014.Conversion.fsproj";             Dll = "Rocksmith2014.Conversion.dll" }
    @{ Path = "src/Rocksmith2014.DD/Rocksmith2014.DD.fsproj";                             Dll = "Rocksmith2014.DD.dll" }
    @{ Path = "src/Rocksmith2014.DD.Model/Rocksmith2014.DD.Model.csproj";                 Dll = "Rocksmith2014.DD.Model.dll" }
    @{ Path = "src/Rocksmith2014.EOF/Rocksmith2014.EOF.fsproj";                           Dll = "Rocksmith2014.EOF.dll" }
)

Write-Host "=== DLCB Dependency Builder ===" -ForegroundColor Cyan

# Step 1: Clone or update repo
if ($Clean -and (Test-Path $RepoDir)) {
    Write-Host "Cleaning existing checkout..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $RepoDir
}

if (-not (Test-Path $RepoDir)) {
    Write-Host "Cloning Rocksmith2014.NET ($Branch)..." -ForegroundColor Green
    git clone --branch $Branch --single-branch $RepoUrl $RepoDir
    if ($LASTEXITCODE -ne 0) { throw "Git clone failed" }
} else {
    Write-Host "Updating existing checkout..." -ForegroundColor Green
    Push-Location $RepoDir
    try {
        git fetch origin
        git checkout $Branch
        git reset --hard "origin/$Branch"
        if ($LASTEXITCODE -ne 0) { throw "Git update failed" }
    } finally {
        Pop-Location
    }
}

# Step 2: Apply patches (PowerShell scripts in patches/ directory)
$Patches = Get-ChildItem -Path $PatchDir -Filter "*.ps1" -ErrorAction SilentlyContinue | Sort-Object Name
if ($Patches.Count -gt 0) {
    Write-Host "`nApplying $($Patches.Count) patch(es)..." -ForegroundColor Green
    foreach ($Patch in $Patches) {
        Write-Host "  Applying: $($Patch.Name)" -ForegroundColor Yellow
        & $Patch.FullName -RepoDir $RepoDir
    }
} else {
    Write-Host "`nNo patches found in $PatchDir" -ForegroundColor Yellow
}

# Step 3: Build
# Upstream sets <TreatWarningsAsErrors>true</TreatWarningsAsErrors> in every project, and NuGet's
# vulnerability audit flags transitive deps (e.g. Magick.NET-Q8-x64) against a live advisory DB.
# That combination turns audit warnings (NU1901/1902/1903) into hard restore/build errors as new
# advisories are published, even when our source is unchanged. We can't fix upstream's transitive
# deps, so disable the audit for this dependency build.
$NoAudit = "-p:NuGetAudit=false"

Write-Host "`nBuilding solution..." -ForegroundColor Green
$SlnPath = Join-Path $RepoDir "Rocksmith2014.sln"
dotnet restore $SlnPath $NoAudit
if ($LASTEXITCODE -ne 0) { throw "Restore failed" }

# Build all projects in Release mode
foreach ($Project in $Projects) {
    $ProjPath = Join-Path $RepoDir $Project.Path
    if (Test-Path $ProjPath) {
        Write-Host "  Building: $($Project.Dll)" -ForegroundColor Yellow
        dotnet build $ProjPath -c Release --no-restore $NoAudit
        if ($LASTEXITCODE -ne 0) { throw "Build failed for $($Project.Dll)" }
    } else {
        Write-Host "  SKIPPING (not found): $($Project.Path)" -ForegroundColor Red
    }
}

# Step 4: Copy DLLs
Write-Host "`nCopying DLLs to Deps/..." -ForegroundColor Green
$Copied = 0
foreach ($Project in $Projects) {
    $DllName = $Project.Dll
    # Find the built DLL in the Release output
    $BuiltDll = Get-ChildItem -Path $RepoDir -Filter $DllName -Recurse |
        Where-Object { $_.FullName -match "bin[\\/]Release" -and $_.FullName -notmatch "ref[\\/]" } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if ($BuiltDll) {
        $Dest = Join-Path $DepsDir $DllName
        Copy-Item $BuiltDll.FullName $Dest -Force
        Write-Host "  Copied: $DllName" -ForegroundColor Yellow
        $Copied++
    } else {
        Write-Host "  NOT FOUND: $DllName" -ForegroundColor Red
    }
}

Write-Host "`n=== Done! Copied $Copied/$($Projects.Count) DLLs ===" -ForegroundColor Cyan
