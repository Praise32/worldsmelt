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

"%APP%"
echo.
echo Codice uscita: %ERRORLEVEL%
pause
popd
