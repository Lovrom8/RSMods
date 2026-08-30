<#
.SYNOPSIS
    Publishes the self-contained, single-file Avalonia installer for Windows x64.

.DESCRIPTION
    The native mod and generated configurator payload must already exist. The output is staged under
    Build/Artifacts/installer-publish/win-x64 and contains the user-facing release executable.
#>
[CmdletBinding()]
param(
    [string] $Configuration = 'Release'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repoRoot 'Installer/Installer.csproj'
$outputDir = Join-Path $repoRoot 'Build/Artifacts/installer-publish/win-x64'

if (Test-Path $outputDir) {
    Remove-Item -Recurse -Force $outputDir
}

& dotnet publish $project `
    --configuration $Configuration `
    -p:PublishProfile=win-x64 `
    --output $outputDir `
    --nologo
if ($LASTEXITCODE -ne 0) {
    throw "Installer publish failed with exit code $LASTEXITCODE."
}

$installer = Join-Path $outputDir 'RS2014-Mod-Installer.exe'
if (-not (Test-Path $installer -PathType Leaf)) {
    throw "Published installer not found at '$installer'."
}

# Referenced projects may still contribute symbols even when the executable's publish profile disables
# them. They are not part of the release artifact.
$pdbs = @(Get-ChildItem -File -Filter '*.pdb' $outputDir -ErrorAction SilentlyContinue)
if ($pdbs.Count -gt 0) {
    $pdbs | Remove-Item -Force
}

$unexpected = @(Get-ChildItem -File $outputDir | Where-Object Name -ne 'RS2014-Mod-Installer.exe')
if ($unexpected.Count -gt 0) {
    throw "Single-file publish produced unexpected loose files: $($unexpected.Name -join ', ')."
}

Write-Host "Installer publish OK: $installer" -ForegroundColor Green
