@echo off
setlocal

set "DEMO_DIR=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%DEMO_DIR%build.ps1"
if errorlevel 1 exit /b %errorlevel%

set "OUTPUT_DIR=%DEMO_DIR%..\..\build\procedural-combat-demo"
set "EXE=%OUTPUT_DIR%\worldsmelt-procedural-combat-demo.exe"
pushd "%OUTPUT_DIR%"
"%EXE%" %*
set "DEMO_STATUS=%ERRORLEVEL%"
popd
exit /b %DEMO_STATUS%

