@echo off
setlocal

pushd "%~dp0" || exit /b 1

where node >NUL 2>NUL
if errorlevel 1 (
    echo ERRORE: Node.js non trovato nel PATH.
    popd
    exit /b 1
)

echo Genero run dinamica con OpenAI text + spritesheet Image API giocabile...
node llm\generate_run.mjs --image %*

set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
