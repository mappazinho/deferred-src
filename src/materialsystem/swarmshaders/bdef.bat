@echo off
setlocal

rem Build Biohazard deferred shaders for this SDK 2013 checkout.
rem Run from a Visual Studio Developer Command Prompt so nmake.exe is available.

pushd "%~dp0"

for %%I in ("%~dp0..\..\..") do set "REPOROOT=%%~fI"
set "GAMEDIR=%REPOROOT%\game\mod_episodic"
set "SOURCEDIR=%REPOROOT%\src"

rem SDKBINDIR may be supplied by the caller. If it is not, try the normal
rem Source SDK Base 2013 Singleplayer install under Steam's primary library.
if not defined SDKBINDIR (
	for /f "tokens=2,*" %%A in ('reg query "HKCU\Software\Valve\Steam" /v SteamPath 2^>nul ^| find /i "SteamPath"') do set "STEAMPATH=%%B"
	if defined STEAMPATH set "SDKBINDIR=%STEAMPATH%\steamapps\common\Source SDK Base 2013 Singleplayer\bin"
)

if not exist "%GAMEDIR%\gameinfo.txt" (
	echo ERROR: Could not find the mod game directory:
	echo        "%GAMEDIR%"
	popd
	exit /b 1
)

if not defined SDKBINDIR goto no_sdk_bin
if not exist "%SDKBINDIR%\shadercompile.exe" goto no_sdk_bin

where nmake.exe >nul 2>nul
if errorlevel 1 (
	echo ERROR: nmake.exe is not on PATH.
	echo Run this script from a Visual Studio Developer Command Prompt.
	popd
	exit /b 1
)

rem buildshaders.bat has legacy assumptions about spaces in SDKBINDIR, so use
rem the short form when Windows provides one.
for %%I in ("%SDKBINDIR%") do set "SDKBINDIR=%%~sI"

set "BUILD_SHADER=call buildshaders.bat"
%BUILD_SHADER% deferred_shaders -game "%GAMEDIR%" -source "%SOURCEDIR%" -dx9_30 -force30
set "BUILD_RESULT=%ERRORLEVEL%"

popd
exit /b %BUILD_RESULT%

:no_sdk_bin
echo ERROR: shadercompile.exe was not found.
echo Set SDKBINDIR to the bin directory that contains shadercompile.exe, for example:
echo   set "SDKBINDIR=C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Singleplayer\bin"
echo Then run bdef.bat again.
popd
exit /b 1
