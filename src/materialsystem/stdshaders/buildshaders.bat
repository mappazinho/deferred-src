@echo off

set TTEXE=..\..\devtools\bin\timeprecise.exe
if not exist %TTEXE% goto no_ttexe
goto no_ttexe_end

:no_ttexe
set TTEXE=time /t
:no_ttexe_end

echo.
echo ==================== buildshaders %* ==================
%TTEXE% -cur-Q
set tt_start=%ERRORLEVEL%
set tt_chkpt=%tt_start%

REM ****************
REM usage: buildshaders <shaderProjectName>
REM ****************

setlocal
set arg_filename=%1
set shadercompilecommand=ShaderCompile.exe
set targetdir=shaders
set SrcDirBase=..\..
set shaderDir=shaders

if "%1" == "" goto usage
set inputbase=%1

REM SCell555/ShaderCompile does not use the stock DXSDK selector.
if /i "%6" == "-dx9_30" shift /6

if /i "%6" == "-force30" goto set_force30_arg
goto set_force_end
:set_force30_arg
set IS30=1
goto set_force_end
:set_force_end

if /i "%2" == "-game" goto set_mod_args
goto build_shaders

REM ****************
REM USAGE
REM ****************
:usage
echo.
echo "usage: buildshaders <shaderProjectName> [-game] [gameDir if -game was specified] [-source sourceDir]"
echo "       gameDir is where gameinfo.txt is (where it will store the compiled shaders)."
echo "       sourceDir is where the source code is (where it will find scripts and compilers)."
echo "ex   : buildshaders myshaders"
echo "ex   : buildshaders myshaders -game c:\steam\steamapps\sourcemods\mymod -source c:\mymod\src"
goto end

REM ****************
REM MOD ARGS
REM ****************
:set_mod_args

if /i "%4" NEQ "-source" goto NoSourceDirSpecified
set SrcDirBase=%~5
set ChangeToDir=%SrcDirBase%\devtools\bin

REM Install the standalone compiler on first use instead of relying on
REM Source SDK Base's stock shadercompile.exe in %%SDKBINDIR%%.
if not exist "%ChangeToDir%\ShaderCompile.exe" (
    echo SCell555 ShaderCompile is not installed; downloading the pinned release...
    powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ChangeToDir%\install_shadercompile.ps1"
    if errorlevel 1 goto ShaderCompileInstallFailed
)
if not exist "%ChangeToDir%\ShaderCompile.exe" goto NoShaderCompile

REM ** use the -game parameter to tell us where to put the files
set targetdir=%~3\shaders

if not exist "%~3\gameinfo.txt" goto InvalidGameDirectory
if not exist "%inputbase%.txt" goto InvalidInputFile
goto build_shaders

REM ****************
REM ERRORS
REM ****************
:InvalidGameDirectory
echo Error: "%~3" is not a valid game directory.
echo (The -game directory must have a gameinfo.txt file)
goto end

:InvalidInputFile
echo Error: "%inputbase%.txt" is not a valid file.
goto end

:NoSourceDirSpecified
echo ERROR: If you specify -game on the command line, you must specify -source.
goto usage

:ShaderCompileInstallFailed
echo ERROR: Failed to install SCell555 ShaderCompile.
echo Run "%SrcDirBase%\devtools\bin\install_shadercompile.ps1" manually for details.
goto end

:NoShaderCompile
echo ERROR: ShaderCompile.exe doesn't exist in %SrcDirBase%\devtools\bin
goto end

REM ****************
REM BUILD SHADERS
REM ****************
:build_shaders

if not exist include mkdir include
if not exist %shaderDir% mkdir %shaderDir%
if not exist %shaderDir%\fxc mkdir %shaderDir%\fxc

set SHVER=20b
if defined IS30 (
    set SHVER=30
)

title %1 %SHVER%

echo Building shader headers and VCS files for %inputbase% with SCell555 ShaderCompile...

set DYNAMIC=
if "%dynamic_shaders%" == "1" set DYNAMIC=-Dynamic
powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%SrcDirBase%\devtools\bin\process_shaders.ps1" %DYNAMIC% -Version %SHVER% "%inputbase%.txt"
if errorlevel 1 goto ShaderBuildFailed

REM ****************
REM PC Shader copy
REM ****************
:DoXCopy
if not "%dynamic_shaders%" == "1" (
    if not exist "%targetdir%" md "%targetdir%"
    if not "%targetdir%"=="%shaderDir%" xcopy %shaderDir%\*.* "%targetdir%" /e /y
)
goto end

:ShaderBuildFailed
echo ERROR: Shader compilation failed for %inputbase%.

REM ****************
REM END
REM ****************
:end

%TTEXE% -diff %tt_start%
echo.
