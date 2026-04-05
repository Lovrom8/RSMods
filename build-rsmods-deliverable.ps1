param(
    [string]$Config = "Release",
    [string]$Platform = "Win32",
    [string]$VersionTag = "qc-dev-local",
    [string]$DistRoot = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path $PSScriptRoot).Path
if ([string]::IsNullOrWhiteSpace($DistRoot)) {
    $DistRoot = Join-Path $repoRoot "dist"
}

$solution = Join-Path $repoRoot "RSMods.sln"
$installReadmeSource = Join-Path $repoRoot "docs\install-readme.md"
$qcReadmeSource = Join-Path $repoRoot "docs\qc-use-readme.md"
$qcArchitectureSource = Join-Path $repoRoot "docs\qc-architecture.md"
$releaseChecklistSource = Join-Path $repoRoot "docs\release-checklist.md"
$releaseTemplateSource = Join-Path $repoRoot "docs\release-notes-template.md"

foreach ($requiredPath in @(
    $solution,
    $installReadmeSource,
    $qcReadmeSource,
    $qcArchitectureSource,
    $releaseChecklistSource,
    $releaseTemplateSource
)) {
    if (-not (Test-Path $requiredPath)) {
        throw "Required path not found: $requiredPath"
    }
}

$msbuild = $null
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
}
if (-not $msbuild) {
    $candidates = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    )
    $msbuild = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $msbuild) {
    throw "MSBuild.exe not found. Install Visual Studio with MSBuild."
}

Write-Host ("Using MSBuild: " + $msbuild)
Write-Host ("Restoring: " + $solution)
& $msbuild $solution /t:Restore /p:RestorePackagesConfig=true /p:Configuration=$Config /p:Platform=$Platform /m
if ($LASTEXITCODE -ne 0) {
    throw "Restore failed with exit code $LASTEXITCODE"
}

Write-Host ("Building: " + $solution + " [" + $Config + "|" + $Platform + "]")
& $msbuild $solution /t:Build /p:Configuration=$Config /p:Platform=$Platform /p:PostBuildEventUseInBuild=false /m
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

$dllCandidates = @(
    (Join-Path $repoRoot "Installer\Resources\xinput1_3.dll"),
    (Join-Path $repoRoot "DLL\Installer\Resources\xinput1_3.dll")
)
$dllPath = $dllCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $dllPath) {
    throw ("xinput1_3.dll not found. Checked: " + ($dllCandidates -join ", "))
}

$guiDir = Join-Path $repoRoot "Installer\Resources\RSModsGUI"
if (-not (Test-Path $guiDir)) {
    throw "GUI resource folder not found: $guiDir"
}

$installerCandidates = @(
    (Join-Path $repoRoot ("Installer\bin\" + $Config + "\RS2014-Mod-Installer.exe")),
    (Join-Path $repoRoot ("Installer\bin\x64\" + $Config + "\RS2014-Mod-Installer.exe"))
)
$installerExe = $installerCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
$installerExists = [bool]$installerExe
if (-not $installerExists) {
    Write-Host ("WARNING: Installer EXE not found. Checked: " + ($installerCandidates -join ", "))
}

if (-not (Test-Path $DistRoot)) {
    New-Item -ItemType Directory -Path $DistRoot -Force | Out-Null
}
$resolvedDistRoot = (Resolve-Path $DistRoot).Path

$artifactBase = "RSMods-QC-$VersionTag"
$fullPackageFolderName = "$artifactBase-win32"
$portablePackageFolderName = "$artifactBase-portable"

$stageRoot = Join-Path $resolvedDistRoot ".stage\$artifactBase"
$fullPackageRoot = Join-Path $stageRoot $fullPackageFolderName
$portablePackageRoot = Join-Path $stageRoot $portablePackageFolderName

$fullInstallerDir = Join-Path $fullPackageRoot "installer"
$fullPortableDir = Join-Path $fullPackageRoot "portable"
$fullPortableRsmodsDir = Join-Path $fullPortableDir "RSMods"
$fullDocsDir = Join-Path $fullPackageRoot "docs"

$portableRsmodsDir = Join-Path $portablePackageRoot "RSMods"
$portableDocsDir = Join-Path $portablePackageRoot "docs"

$win32ZipPath = Join-Path $resolvedDistRoot "$fullPackageFolderName.zip"
$portableZipPath = Join-Path $resolvedDistRoot "$portablePackageFolderName.zip"
$installerOutPath = Join-Path $resolvedDistRoot "$artifactBase-installer.exe"
$checksumPath = Join-Path $resolvedDistRoot "SHA256SUMS.txt"

if (Test-Path $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
foreach ($file in @($win32ZipPath, $portableZipPath, $installerOutPath, $checksumPath)) {
    if (Test-Path $file) {
        Remove-Item -LiteralPath $file -Force
    }
}

New-Item -ItemType Directory -Path $fullInstallerDir -Force | Out-Null
New-Item -ItemType Directory -Path $fullPortableRsmodsDir -Force | Out-Null
New-Item -ItemType Directory -Path $fullDocsDir -Force | Out-Null
New-Item -ItemType Directory -Path $portableRsmodsDir -Force | Out-Null
New-Item -ItemType Directory -Path $portableDocsDir -Force | Out-Null

Copy-Item -LiteralPath $dllPath -Destination (Join-Path $fullPortableDir "xinput1_3.dll") -Force
Copy-Item -Path (Join-Path $guiDir "*") -Destination $fullPortableRsmodsDir -Recurse -Force
Copy-Item -LiteralPath $dllPath -Destination (Join-Path $portablePackageRoot "xinput1_3.dll") -Force
Copy-Item -Path (Join-Path $guiDir "*") -Destination $portableRsmodsDir -Recurse -Force

Copy-Item -LiteralPath $installReadmeSource -Destination (Join-Path $fullDocsDir "INSTALL-README.md") -Force
Copy-Item -LiteralPath $qcReadmeSource -Destination (Join-Path $fullDocsDir "QC-USE-README.md") -Force
Copy-Item -LiteralPath $qcArchitectureSource -Destination (Join-Path $fullDocsDir "QC-ARCHITECTURE.md") -Force
Copy-Item -LiteralPath $releaseChecklistSource -Destination (Join-Path $fullDocsDir "RELEASE-CHECKLIST.md") -Force
Copy-Item -LiteralPath $releaseTemplateSource -Destination (Join-Path $fullDocsDir "RELEASE-NOTES-TEMPLATE.md") -Force
Copy-Item -LiteralPath $installReadmeSource -Destination (Join-Path $portableDocsDir "INSTALL-README.md") -Force
Copy-Item -LiteralPath $qcReadmeSource -Destination (Join-Path $portableDocsDir "QC-USE-README.md") -Force
Copy-Item -LiteralPath $qcArchitectureSource -Destination (Join-Path $portableDocsDir "QC-ARCHITECTURE.md") -Force

if ($installerExists) {
    $installerConfig = $installerExe + ".config"
    Copy-Item -LiteralPath $installerExe -Destination (Join-Path $fullInstallerDir "RS2014-Mod-Installer.exe") -Force
    if (Test-Path $installerConfig) {
        Copy-Item -LiteralPath $installerConfig -Destination (Join-Path $fullInstallerDir "RS2014-Mod-Installer.exe.config") -Force
    }
    Copy-Item -LiteralPath $installerExe -Destination $installerOutPath -Force
}

$gitCommit = (& git -C $repoRoot rev-parse --short HEAD 2>$null)
if ($LASTEXITCODE -ne 0) {
    $gitCommit = "unknown"
}

$buildInfo = @(
    "Build tag: $VersionTag",
    "Build config: $Config | $Platform",
    ("Built at: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss")),
    "Repo root: $repoRoot",
    "Git commit: $gitCommit",
    "Installer included: $installerExists"
)

Set-Content -LiteralPath (Join-Path $fullPackageRoot "BUILD-INFO.txt") -Value $buildInfo -Encoding UTF8
Set-Content -LiteralPath (Join-Path $portablePackageRoot "BUILD-INFO.txt") -Value $buildInfo -Encoding UTF8

Compress-Archive -Path $fullPackageRoot -DestinationPath $win32ZipPath -CompressionLevel Optimal
Compress-Archive -Path $portablePackageRoot -DestinationPath $portableZipPath -CompressionLevel Optimal

$checksumTargets = @($win32ZipPath, $portableZipPath)
if ($installerExists -and (Test-Path $installerOutPath)) {
    $checksumTargets += $installerOutPath
}

$checksumLines = @()
foreach ($file in $checksumTargets) {
    $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    $checksumLines += "$hash *$(Split-Path -Leaf $file)"
}
Set-Content -LiteralPath $checksumPath -Value $checksumLines -Encoding ASCII

if (Test-Path $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
$stageParent = Join-Path $resolvedDistRoot ".stage"
if (Test-Path $stageParent) {
    $remainingStageEntries = Get-ChildItem -LiteralPath $stageParent -Force
    if ($remainingStageEntries.Count -eq 0) {
        Remove-Item -LiteralPath $stageParent -Force
    }
}

Write-Host "--- Deliverable Summary ---"
Write-Host ("Version tag:  " + $VersionTag)
Write-Host ("Dist folder:  " + $resolvedDistRoot)
Write-Host ("Win32 ZIP:    " + $win32ZipPath)
Write-Host ("Portable ZIP: " + $portableZipPath)
if ($installerExists -and (Test-Path $installerOutPath)) {
    Write-Host ("Installer EXE: " + $installerOutPath)
}
else {
    Write-Host "Installer EXE: not found in build output (skipped)."
}
Write-Host ("Checksums:    " + $checksumPath)
