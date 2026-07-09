@echo off
setlocal

pushd "%~dp0" || exit /b 1

call "%~dp0generate_dynamic_assets.bat" %*
if errorlevel 1 (
    echo.
    echo Generazione dinamica fallita. Avvio interrotto per evitare costi o asset incoerenti.
    pause
    popd
    exit /b 1
)

call "%~dp0build_gpu.bat"
if errorlevel 1 (
    echo.
    echo Build GPU fallita.
    pause
    popd
    exit /b 1
)

start "Melting Run Dynamic GPU" /D "%~dp0" "%~dp0bin\melting_run_gpu.exe"
popd
