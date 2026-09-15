@echo off
setlocal
set VSDIR=C:\Program Files\Microsoft Visual Studio\18\Community
call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" amd64_x86
cd /d "%~dp0"
set MSB=%VSDIR%\MSBuild\Current\Bin\MSBuild.exe
set LOGDIR=%~dp0buildlogs
if not exist "%LOGDIR%" mkdir "%LOGDIR%"

for %%P in (mathlib\mathlib tier1\tier1 vgui2\vgui_controls\vgui_controls materialsystem\swarmshaders\game_shader_dx9_episodic game\gameui2\gameui2 game\client\client_episodic game\server\server_episodic) do (
    echo ===== BUILDING %%P =====
    "%MSB%" %%P.vcxproj /p:Configuration=Release /p:Platform=Win32 /m /v:m /nologo /flp:"LogFile=%LOGDIR%\%%~nP.log;Verbosity=normal" /flp1:"LogFile=%LOGDIR%\%%~n-errors.log;Verbosity=minimal;ErrorSummaryOnly"
    if errorlevel 1 (
        echo ===== FAILED: %%P =====
    ) else (
        echo ===== OK: %%P =====
    )
)
endlocal
