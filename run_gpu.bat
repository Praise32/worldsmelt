@echo off
setlocal

pushd "%~dp0" || exit /b 1

set "APP=%~dp0bin\melting_run_gpu.exe"

if not exist "%APP%" (
    echo Eseguibile GPU non trovato. Provo a compilarlo...
    call "%~dp0build_gpu.bat"
    if errorlevel 1 (
        echo.
        echo Build GPU fallita. Controlla l'errore sopra.
        pause
        popd
        exit /b 1
    )
)

tasklist /FI "IMAGENAME eq melting_run_gpu.exe" 2>NUL | find /I "melting_run_gpu.exe" >NUL
if not errorlevel 1 (
    echo Melting Run GPU e' gia' in esecuzione.
    ping -n 3 127.0.0.1 >NUL
    popd
    exit /b 0
)

start "Melting Run GPU" /D "%~dp0" "%APP%"
popd
