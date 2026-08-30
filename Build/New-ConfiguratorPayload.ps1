<#
.SYNOPSIS
    Packages the published Avalonia configurator into a single installer payload plus a manifest.

.DESCRIPTION
    Slice 3 of docs/winforms-retirement-plan.md. Consumes the output of Build/Publish-Configurator.ps1
    (Build/Artifacts/configurator/win-x64) and produces:

        Build/Artifacts/installer/RSModsGUI.zip
        Build/Artifacts/installer/RSModsGUI.manifest.json

    The manifest fully describes the installation: a payload format version, the application version, and
    the relative path / length / SHA-256 of every file, the executable path, and the directories owned by
    the payload. All archive paths are normalized to '/'; absolute paths and '..' segments are rejected.

    After writing the archive the script self-verifies by extracting to a temporary directory and checking
    every manifest hash, so a corrupt payload fails here rather than at install time. Pass -VerifyOnly to
    run just that validation against an already-generated payload (used as the installer build's gate).

.EXAMPLE
    pwsh Build/Publish-Configurator.ps1
    pwsh Build/New-ConfiguratorPayload.ps1

.EXAMPLE
    pwsh Build/New-ConfiguratorPayload.ps1 -VerifyOnly
#>
[CmdletBinding()]
param(
    [switch] $VerifyOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# ZipFile/ZipFileExtensions live in the FileSystem assembly; ZipArchive/ZipArchiveMode/CompressionLevel
# live in System.IO.Compression. Load both so this works under Windows PowerShell 5.1 and pwsh alike.
Add-Type -AssemblyName System.IO.Compression | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem | Out-Null

$PayloadFormatVersion = 1

$repoRoot   = Split-Path -Parent $PSScriptRoot
$publishDir = Join-Path $repoRoot 'Build/Artifacts/configurator/win-x64'
$installDir = Join-Path $repoRoot 'Build/Artifacts/installer'
$zipPath    = Join-Path $installDir 'RSModsGUI.zip'
$manifest   = Join-Path $installDir 'RSModsGUI.manifest.json'
$exeName    = 'RSMods.exe'

# Relative path with forward slashes; reject anything that could escape the install directory.
# Computed by hand (not [IO.Path]::GetRelativePath, which is absent from Windows PowerShell 5.1) since
# every file is known to live under $Root.
function Get-SafeRelativePath {
    param([string] $Root, [string] $FullPath)
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar)
    $full     = [IO.Path]::GetFullPath($FullPath)
    $prefix   = $rootFull + [IO.Path]::DirectorySeparatorChar
    if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path '$full' is not under payload root '$rootFull'."
    }
    $rel = $full.Substring($prefix.Length) -replace '\\', '/'
    if ([IO.Path]::IsPathRooted($rel) -or $rel -eq '..' -or $rel.StartsWith('../') -or $rel -match '(^|/)\.\.(/|$)') {
        throw "Unsafe payload path rejected: '$rel'"
    }
    return $rel
}

function Get-Sha256 {
    param([string] $Path)
    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
}

function Read-Manifest {
    if (-not (Test-Path $manifest -PathType Leaf)) {
        throw "Manifest not found at '$manifest'. Generate the payload first."
    }
    return Get-Content -Raw -LiteralPath $manifest | ConvertFrom-Json
}

# --- Verification (extract to temp, check every hash) --------------------------------------------

function Test-Payload {
    if (-not (Test-Path $zipPath -PathType Leaf)) {
        throw "Payload archive not found at '$zipPath'. Generate the payload first."
    }
    $m = Read-Manifest
    $temp = Join-Path ([IO.Path]::GetTempPath()) ("RSModsPayload_" + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $temp | Out-Null
    try {
        [IO.Compression.ZipFile]::ExtractToDirectory($zipPath, $temp)
        $problems = @()
        foreach ($entry in $m.files) {
            # Re-validate the recorded path before touching disk.
            if ([IO.Path]::IsPathRooted($entry.path) -or $entry.path -match '(^|/)\.\.(/|$)') {
                $problems += "unsafe path in manifest: $($entry.path)"
                continue
            }
            $extracted = Join-Path $temp ($entry.path -replace '/', [IO.Path]::DirectorySeparatorChar)
            if (-not (Test-Path $extracted -PathType Leaf)) {
                $problems += "missing after extract: $($entry.path)"
                continue
            }
            if ((Get-Item -LiteralPath $extracted).Length -ne $entry.length) {
                $problems += "length mismatch: $($entry.path)"
                continue
            }
            if ((Get-Sha256 $extracted) -ne $entry.sha256) {
                $problems += "hash mismatch: $($entry.path)"
            }
        }
        if ($problems.Count -gt 0) {
            Write-Host 'Payload verification FAILED:' -ForegroundColor Red
            $problems | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
            throw "Payload verification failed ($($problems.Count) problem(s))."
        }
        Write-Host "Payload verified: $($m.files.Count) files match the manifest." -ForegroundColor Green
    }
    finally {
        Remove-Item -Recurse -Force $temp -ErrorAction SilentlyContinue
    }
}

if ($VerifyOnly) {
    Test-Payload
    return
}

# --- Generation ----------------------------------------------------------------------------------

if (-not (Test-Path $publishDir -PathType Container)) {
    throw "Publish output not found at '$publishDir'. Run Build/Publish-Configurator.ps1 first."
}

$exePath = Join-Path $publishDir $exeName
if (-not (Test-Path $exePath -PathType Leaf)) {
    throw "Executable '$exeName' not found in the publish output."
}

if (Test-Path $installDir) {
    Remove-Item -Recurse -Force $installDir
}
New-Item -ItemType Directory -Path $installDir | Out-Null

$appVersion = (Get-Item -LiteralPath $exePath).VersionInfo.ProductVersion
if ([string]::IsNullOrWhiteSpace($appVersion)) { $appVersion = '0.0.0' }

# Enumerate the payload deterministically (sorted by relative path). Entries are [pscustomobject] in a
# plain array - ConvertTo-Json serializes those cleanly and preserves property order.
$files = Get-ChildItem -Recurse -File $publishDir | Sort-Object FullName
$fileEntries = @()
$dirSet = New-Object 'System.Collections.Generic.HashSet[string]'

foreach ($f in $files) {
    $rel = Get-SafeRelativePath -Root $publishDir -FullPath $f.FullName
    $fileEntries += [pscustomobject]@{
        path   = $rel
        length = $f.Length
        sha256 = Get-Sha256 $f.FullName
    }
    $parent = Split-Path $rel -Parent
    while (-not [string]::IsNullOrEmpty($parent)) {
        [void]$dirSet.Add(($parent -replace '\\', '/'))
        $parent = Split-Path $parent -Parent
    }
}

$manifestObj = [pscustomobject]@{
    payloadFormatVersion = $PayloadFormatVersion
    application          = 'RSMods'
    applicationVersion   = $appVersion
    executable           = $exeName
    directories          = @($dirSet | Sort-Object)
    files                = @($fileEntries)
}

# Build the zip with forward-slash entry names (do not rely on CreateFromDirectory's OS separators).
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
$archive = [IO.Compression.ZipFile]::Open($zipPath, [IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($entry in $fileEntries) {
        $source = Join-Path $publishDir ($entry.path -replace '/', [IO.Path]::DirectorySeparatorChar)
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $source, $entry.path, [IO.Compression.CompressionLevel]::Optimal)
    }
}
finally {
    $archive.Dispose()
}

# Write UTF-8 without a BOM: Set-Content -Encoding UTF8 on Windows PowerShell 5.1 prepends one, which some
# strict JSON readers choke on. .NET's UTF8Encoding($false) gives clean bytes on 5.1 and pwsh alike.
$json = $manifestObj | ConvertTo-Json -Depth 5
[IO.File]::WriteAllText($manifest, $json, (New-Object System.Text.UTF8Encoding($false)))

Write-Host "Payload:  $zipPath"
Write-Host "Manifest: $manifest ($($fileEntries.Count) files, version $appVersion)"

# Self-verify before declaring success.
Test-Payload
