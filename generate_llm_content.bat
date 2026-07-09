@echo off
setlocal

pushd "%~dp0" || exit /b 1

where node >NUL 2>NUL
if errorlevel 1 (
    echo ERRORE: Node.js non trovato nel PATH.
    echo Installa Node.js oppure usa il fallback procedurale del gioco.
    popd
    exit /b 1
)

if "%~1"=="" (
    node llm\generate_run.mjs
) else (
    node llm\generate_run.mjs %*
)

set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
