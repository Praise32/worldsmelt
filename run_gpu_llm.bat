@echo off
setlocal

pushd "%~dp0" || exit /b 1

echo Genero contenuti LLM/cache per la run...
call "%~dp0generate_llm_content.bat" %*
if errorlevel 1 (
    echo.
    echo Generazione fallita. Avvio comunque il gioco con fallback interno se possibile.
)

call "%~dp0build_gpu.bat"
if errorlevel 1 (
    echo.
    echo Build GPU fallita.
    pause
    popd
    exit /b 1
)

start "Melting Run LLM GPU" /D "%~dp0" "%~dp0bin\melting_run_gpu.exe"
popd
