@echo off
setlocal

rem Build Biohazard deferred shaders for this SDK 2013 checkout using
rem SCell555 ShaderCompile through the shared stdshaders build wrapper.

pushd "%~dp0"

for %%I in ("%~dp0..\..\..") do set "REPOROOT=%%~fI"
set "GAMEDIR=%REPOROOT%\game\mod_episodic"
set "SOURCEDIR=%REPOROOT%\src"

if not exist "%GAMEDIR%\gameinfo.txt" (
	echo ERROR: Could not find the mod game directory:
	echo        "%GAMEDIR%"
	popd
	exit /b 1
)

rem Keep the working directory in swarmshaders so deferred_shaders.txt and
rem all referenced deferred shader sources resolve exactly as before. The
rem shared stdshaders wrapper now handles installing/running SCell555's
rem ShaderCompile, so bdef no longer depends on SDK shadercompile, nmake,
rem Perl, or the legacy -dx9_30 path.
call "..\stdshaders\buildshaders.bat" deferred_shaders -game "%GAMEDIR%" -source "%SOURCEDIR%" -force30
set "BUILD_RESULT=%ERRORLEVEL%"

popd
exit /b %BUILD_RESULT%
