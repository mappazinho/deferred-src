@echo off
rem ============================================================================
rem copytosteam.bat
rem Deploys the freshly built deferred mod (mod_episodic) into the Steam install
rem so it can be run directly from Steam. Adjust the variables below to match
rem your machine, then run this after a successful build.
rem ============================================================================

setlocal

rem --- Root of your Steam library that contains sourcemods -------------------
set STEAM_LIBRARY=C:\Program Files (x86)\Steam

rem --- Mod folder inside the Steam library (sourcemods install) --------------
set DEST=%STEAM_LIBRARY%\steamapps\sourcemods\mod_episodic

rem --- Built mod folder in this repository -----------------------------------
set SRC=%~dp0..\game\mod_episodic

echo Deploying %SRC% -^> %DEST%
echo.

if not exist "%SRC%\gameinfo.txt" (
    echo ERROR: %SRC%\gameinfo.txt not found. Is this the right source mod?
    exit /b 1
)

rem --- Create the destination and its bin folder -----------------------------
if not exist "%DEST%" mkdir "%DEST%"
if not exist "%DEST%\bin" mkdir "%DEST%\bin"

rem --- Copy everything except PDBs, saves and runtime stats ------------------
robocopy "%SRC%" "%DEST%" /E /XD "%SRC%\save" /XF *.pdb demoheader.tmp detail.vbsp ep1_gamestats.dat Gamestate.txt modelsounds.cache stats.txt voice_ban.dt /NFL /NDL /NJH /NJS >nul

rem --- Re-copy the freshly built DLLs to guarantee latest bits ---------------
copy /y "%SRC%\bin\client.dll" "%DEST%\bin\client.dll" >nul
copy /y "%SRC%\bin\server.dll" "%DEST%\bin\server.dll" >nul
copy /y "%SRC%\bin\game_shader_dx9.dll" "%DEST%\bin\game_shader_dx9.dll" >nul
copy /y "%SRC%\bin\gameui2.dll" "%DEST%\bin\gameui2.dll" >nul

echo.
echo Done. Deployed mod to %DEST%
endlocal
pause

