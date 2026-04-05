@echo off
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

set "PLATFORM=%~2"
if "%PLATFORM%"=="" set "PLATFORM=Win32"

set "DIST_ROOT=%~3"
if "%DIST_ROOT%"=="" set "DIST_ROOT=%~dp0dist"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop';" ^
  "$repoRoot = Resolve-Path '%~dp0';" ^
  "$solution = Join-Path $repoRoot 'RSMods.sln';" ^
  "$installReadmeSource = Join-Path $repoRoot 'docs\install-readme.md';" ^
  "$qcReadmeSource = Join-Path $repoRoot 'docs\qc-use-readme.md';" ^
  "$distRoot = '%DIST_ROOT%';" ^
  "if (-not (Test-Path $solution)) { throw 'Solution not found: ' + $solution };" ^
  "if (-not (Test-Path $installReadmeSource)) { throw 'Install readme source not found: ' + $installReadmeSource };" ^
  "if (-not (Test-Path $qcReadmeSource)) { throw 'QC readme source not found: ' + $qcReadmeSource };" ^
  "$msbuild = $null;" ^
  "$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe';" ^
  "if (Test-Path $vswhere) {" ^
  "  $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1;" ^
  "}" ^
  "if (-not $msbuild) {" ^
  "  $candidates = @(" ^
  "    'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'," ^
  "    'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'," ^
  "    'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'" ^
  "  );" ^
  "  $msbuild = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1;" ^
  "}" ^
  "if (-not $msbuild) { throw 'MSBuild.exe not found. Install Visual Studio with MSBuild.' };" ^
  "Write-Host ('Using MSBuild: ' + $msbuild);" ^
  "Write-Host ('Restoring: ' + $solution);" ^
  "& $msbuild $solution /t:Restore /p:RestorePackagesConfig=true /p:Configuration='%CONFIG%' /p:Platform='%PLATFORM%' /m;" ^
  "if ($LASTEXITCODE -ne 0) { throw ('Restore failed with exit code ' + $LASTEXITCODE) };" ^
  "Write-Host ('Building: ' + $solution + ' [' + '%CONFIG%' + '|' + '%PLATFORM%' + ']');" ^
  "& $msbuild $solution /t:Build /p:Configuration='%CONFIG%' /p:Platform='%PLATFORM%' /p:PostBuildEventUseInBuild=false /m;" ^
  "if ($LASTEXITCODE -ne 0) { throw ('Build failed with exit code ' + $LASTEXITCODE) };" ^
  "$dllCandidates = @(" ^
  "  (Join-Path $repoRoot 'Installer\Resources\xinput1_3.dll')," ^
  "  (Join-Path $repoRoot 'DLL\Installer\Resources\xinput1_3.dll')" ^
  ");" ^
  "$dllPath = $dllCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1;" ^
  "if (-not $dllPath) { throw ('xinput1_3.dll not found. Checked: ' + ($dllCandidates -join ', ')) };" ^
  "$guiDir = Join-Path $repoRoot 'Installer\Resources\RSModsGUI';" ^
  "if (-not (Test-Path $guiDir)) { throw 'GUI resource folder not found: ' + $guiDir };" ^
  "$installerCandidates = @(" ^
  "  (Join-Path $repoRoot ('Installer\bin\' + '%CONFIG%' + '\RS2014-Mod-Installer.exe'))," ^
  "  (Join-Path $repoRoot ('Installer\bin\x64\' + '%CONFIG%' + '\RS2014-Mod-Installer.exe'))" ^
  ");" ^
  "$installerExe = $installerCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1;" ^
  "if (-not $installerExe) { throw ('Installer EXE not found. Checked: ' + ($installerCandidates -join ', ')) };" ^
  "if (-not (Test-Path $distRoot)) { New-Item -ItemType Directory -Path $distRoot -Force | Out-Null };" ^
  "$resolvedDistRoot = (Resolve-Path $distRoot).Path;" ^
  "$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss';" ^
  "$packageName = 'RSMods-QC-' + '%CONFIG%' + '-' + $timestamp;" ^
  "$stageRoot = Join-Path $resolvedDistRoot $packageName;" ^
  "$installerOutDir = Join-Path $stageRoot 'installer';" ^
  "$portableRoot = Join-Path $stageRoot 'portable';" ^
  "$portableRsmodsDir = Join-Path $portableRoot 'RSMods';" ^
  "$docsDir = Join-Path $stageRoot 'docs';" ^
  "New-Item -ItemType Directory -Path $installerOutDir -Force | Out-Null;" ^
  "New-Item -ItemType Directory -Path $portableRoot -Force | Out-Null;" ^
  "New-Item -ItemType Directory -Path $portableRsmodsDir -Force | Out-Null;" ^
  "New-Item -ItemType Directory -Path $docsDir -Force | Out-Null;" ^
  "Copy-Item -LiteralPath $installerExe -Destination (Join-Path $installerOutDir 'RS2014-Mod-Installer.exe') -Force;" ^
  "$installerConfig = $installerExe + '.config';" ^
  "if (Test-Path $installerConfig) { Copy-Item -LiteralPath $installerConfig -Destination (Join-Path $installerOutDir 'RS2014-Mod-Installer.exe.config') -Force };" ^
  "Copy-Item -LiteralPath $dllPath -Destination (Join-Path $portableRoot 'xinput1_3.dll') -Force;" ^
  "Copy-Item -Path (Join-Path $guiDir '*') -Destination $portableRsmodsDir -Recurse -Force;" ^
  "Copy-Item -LiteralPath $installReadmeSource -Destination (Join-Path $stageRoot 'README-INSTALL.md') -Force;" ^
  "Copy-Item -LiteralPath $installReadmeSource -Destination (Join-Path $docsDir 'INSTALL-README.md') -Force;" ^
  "Copy-Item -LiteralPath $qcReadmeSource -Destination (Join-Path $docsDir 'QC-AUTOMATION-README.md') -Force;" ^
  "Copy-Item -LiteralPath $qcReadmeSource -Destination (Join-Path $portableRsmodsDir 'QC-AUTOMATION-README.md') -Force;" ^
  "$buildInfoPath = Join-Path $stageRoot 'BUILD-INFO.txt';" ^
  "$buildInfo = @(" ^
  "  'Built from: ' + $repoRoot.Path," ^
  "  'Build config: ' + '%CONFIG%' + ' | ' + '%PLATFORM%'," ^
  "  'Built at: ' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" ^
  ");" ^
  "Set-Content -LiteralPath $buildInfoPath -Value $buildInfo -Encoding UTF8;" ^
  "$zipPath = Join-Path $resolvedDistRoot ($packageName + '.zip');" ^
  "if (Test-Path $zipPath) { Remove-Item -LiteralPath $zipPath -Force };" ^
  "Compress-Archive -Path $stageRoot -DestinationPath $zipPath -CompressionLevel Optimal;" ^
  "Write-Host '--- Deliverable Summary ---';" ^
  "Write-Host ('Stage folder: ' + $stageRoot);" ^
  "Write-Host ('ZIP package:  ' + $zipPath);" ^
  "Write-Host ('Installer:    ' + (Join-Path $installerOutDir 'RS2014-Mod-Installer.exe'));" ^
  "Write-Host ('Portable DLL: ' + (Join-Path $portableRoot 'xinput1_3.dll'));" ^
  "Write-Host ('Portable GUI: ' + $portableRsmodsDir);" ^
  "Write-Host ('Install README: ' + (Join-Path $stageRoot 'README-INSTALL.md'));" ^
  "Write-Host ('QC README:    ' + (Join-Path $docsDir 'QC-AUTOMATION-README.md'));" ^
  "exit 0"

set "EXITCODE=%ERRORLEVEL%"
if not "%EXITCODE%"=="0" (
  echo Deliverable build failed with exit code %EXITCODE%.
)
exit /b %EXITCODE%
