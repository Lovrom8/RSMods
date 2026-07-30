@echo off
setlocal

rem Build the shifter test tools. Double-clickable: locates Visual Studio via
rem vswhere and sets up the x64 build environment itself.
rem   harness.exe    - console measurements (synthetic battery + WAV mode)
rem   ShifterLab.exe - GUI for listening: test signals dry/shifted, live input

where cl >nul 2>&1
if %errorlevel%==0 goto :build

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
	echo Could not find vswhere.exe. Run this from a Developer Command Prompt instead.
	pause
	exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
	echo Could not find a Visual Studio installation with C++ tools.
	pause
	exit /b 1
)

call "%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul

:build
rem /I . comes first so the local stdafx.h stand-in shadows the DLL's real one,
rem keeping Detours pragmas and MIDI pedal initializers out of the test build.
set FLAGS=/nologo /O2 /EHsc /std:c++17 /D_CRT_SECURE_NO_WARNINGS /I . /I ..\..\DLL\Audio

cl %FLAGS% harness.cpp ..\..\DLL\Audio\DelayLinePitchShifter.cpp /Fe:harness.exe
if %errorlevel% neq 0 (
	echo.
	echo harness.exe build failed.
	pause
	exit /b 1
)

cl %FLAGS% ShifterLab.cpp ..\..\DLL\Audio\DelayLinePitchShifter.cpp /Fe:ShifterLab.exe /link /SUBSYSTEM:WINDOWS winmm.lib user32.lib gdi32.lib
if %errorlevel% neq 0 (
	echo.
	echo ShifterLab.exe build failed.
	pause
	exit /b 1
)

echo.
echo Built harness.exe and ShifterLab.exe
echo   Synthetic battery: harness.exe
echo   Recorded DI take:  harness.exe input.wav
echo   Custom shift:      harness.exe input.wav -2 [output.wav]
echo   Listening GUI:     ShifterLab.exe
pause
