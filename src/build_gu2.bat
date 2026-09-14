@echo off
setlocal
set VSDIR=C:\Program Files\Microsoft Visual Studio\18\Community
call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" amd64_x86
cd /d "%~dp0"
set MSB=%VSDIR%\MSBuild\Current\Bin\MSBuild.exe
"%MSB%" game\gameui2\gameui2.vcxproj /p:Configuration=Release /p:Platform=Win32 /m /v:m /nologo /flp:"LogFile=buildlogs\gameui2.log;Verbosity=normal"
if errorlevel 1 goto :eof
"%MSB%" game\client\client_episodic.vcxproj /p:Configuration=Release /p:Platform=Win32 /m /v:m /nologo /flp:"LogFile=buildlogs\client_episodic.log;Verbosity=normal"
endlocal
