@echo off
setlocal

pushd "%~dp0" || exit /b 1

where node >NUL 2>NUL
if errorlevel 1 (
    echo ERRORE: Node.js non trovato nel PATH.
    pause
    popd
    exit /b 1
)

echo Avvio LLM sidecar locale...
echo Usa http://127.0.0.1:8787/generate?seed=123 per generare una run.
node llm\server.mjs

popd
