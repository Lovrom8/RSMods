<#
.SYNOPSIS
    Produces the self-contained win-x64 publish of the Avalonia RSMods configurator.

.DESCRIPTION
    Cutover publish step from docs/winforms-retirement-plan.md (Slice 2). The output is the exact file set
    the installer payload is built from, so this script is deliberately strict:

      * it removes only its own known staging directory (Build/Artifacts/configurator/win-x64);
      * it runs `dotnet publish` in Release with the win-x64 self-contained profile;
      * it never copies from a previous bin/ or Installer/Resources directory - the publish is produced
        fresh by the SDK;
      * it fails if RSMods.exe or any required runtime asset (ddc/, tools/, key natives) is missing.

    The result is self-contained: end users do not need a separately installed .NET runtime.

.EXAMPLE
    pwsh Build/Publish-Configurator.ps1
#>
[CmdletBinding()]
param(
    [string] $Configuration = 'Release'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot   = Split-Path -Parent $PSScriptRoot
$project    = Join-Path $repoRoot 'GUI.Avalonia/GUI.Avalonia.csproj'
$outputDir  = Join-Path $repoRoot 'Build/Artifacts/configurator/win-x64'

if (-not (Test-Path $project)) {
    throw "Configurator project not found at '$project'."
}

# Remove only our known staging directory - never a shared bin/ or installer resource tree.
if (Test-Path $outputDir) {
    Write-Host "Cleaning staging directory: $outputDir"
    Remove-Item -Recurse -Force $outputDir
}

Write-Host "Publishing $project (win-x64, self-contained, $Configuration)..."
& dotnet publish $project `
    --configuration $Configuration `
    -p:PublishProfile=win-x64 `
    --output $outputDir `
    --nologo
if ($LASTEXITCODE -ne 0) {
    throw "dotnet publish failed with exit code $LASTEXITCODE."
}

# Drop debug symbols from the release payload. The profile suppresses the app's own symbols, but a
# referenced project (GUI.Core) still emits a .pdb that publish copies; the shipped configurator has no
# use for it.
$pdbs = @(Get-ChildItem -Recurse -File -Filter '*.pdb' $outputDir -ErrorAction SilentlyContinue)
if ($pdbs.Count -gt 0) {
    $pdbs | Remove-Item -Force
    Write-Host "Removed $($pdbs.Count) debug symbol file(s) from the payload."
}

# --- Validate the published file set -------------------------------------------------------------

# Files that MUST exist for the configurator to run. Relative to $outputDir, using forward slashes.
$requiredFiles = @(
    'RSMods.exe',                    # the configurator executable (installed-path contract)
    '7z64.dll',                      # SevenZipSharp native, loaded by path beside the exe
    'RocksmithToolkitLib.dll',       # toolkit managed assembly
    'Rocksmith2014PsarcLib.dll',     # psarc managed assembly
    'ddc/ddc.exe',                   # DDC tool
    'tools/ww2ogg.exe',              # Wwise -> ogg toolchain
    'tools/revorb.exe'
)
# Directories that must be present and non-empty (relative layout preserved beside the exe).
$requiredDirs = @('ddc', 'tools')

$missing = @()

foreach ($rel in $requiredFiles) {
    $path = Join-Path $outputDir ($rel -replace '/', [IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path $path -PathType Leaf)) {
        $missing += "file: $rel"
    }
}

foreach ($rel in $requiredDirs) {
    $path = Join-Path $outputDir $rel
    if (-not (Test-Path $path -PathType Container)) {
        $missing += "directory: $rel/"
    }
    elseif (-not (Get-ChildItem -Force -File $path -ErrorAction SilentlyContinue)) {
        $missing += "directory (empty): $rel/"
    }
}

if ($missing.Count -gt 0) {
    Write-Host ''
    Write-Host 'Publish output is INCOMPLETE. Missing required items:' -ForegroundColor Red
    $missing | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    throw "Configurator publish validation failed ($($missing.Count) missing item(s))."
}

$fileCount = (Get-ChildItem -Recurse -File $outputDir).Count
Write-Host ''
Write-Host "Publish OK: $fileCount files in $outputDir" -ForegroundColor Green
Write-Host 'Self-contained: the target machine does not need a separately installed .NET runtime.'
