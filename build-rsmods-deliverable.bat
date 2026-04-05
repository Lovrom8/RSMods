@echo off
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

set "PLATFORM=%~2"
if "%PLATFORM%"=="" set "PLATFORM=Win32"

set "VERSION_TAG=%~3"
if "%VERSION_TAG%"=="" set "VERSION_TAG=qc-dev-local"

set "DIST_ROOT=%~4"
if "%DIST_ROOT%"=="" set "DIST_ROOT=%~dp0dist"

set "SCRIPT_PATH=%~dp0build-rsmods-deliverable.ps1"
if not exist "%SCRIPT_PATH%" (
  echo Missing helper script: %SCRIPT_PATH%
  exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" ^
  -Config "%CONFIG%" ^
  -Platform "%PLATFORM%" ^
  -VersionTag "%VERSION_TAG%" ^
  -DistRoot "%DIST_ROOT%"

set "EXITCODE=%ERRORLEVEL%"
if not "%EXITCODE%"=="0" (
  echo Deliverable build failed with exit code %EXITCODE%.
)
exit /b %EXITCODE%
