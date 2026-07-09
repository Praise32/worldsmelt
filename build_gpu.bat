@echo off
setlocal

set "ROOT=%~dp0..\.."
set "GCC=C:\Users\maria\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe"
set "RAYLIB_BUILD=%ROOT%\raylib\build\mingw-debug\raylib"

if not exist "%GCC%" (
    echo ERRORE: gcc non trovato in "%GCC%".
    exit /b 1
)

if not exist "%RAYLIB_BUILD%\libraylib.a" (
    echo ERRORE: libraylib.a non trovato in "%RAYLIB_BUILD%".
    exit /b 1
)

if not exist "%~dp0bin" mkdir "%~dp0bin"

set "SOURCE_LIST=%TEMP%\melting-run-gpu-sources-%RANDOM%.txt"
> "%SOURCE_LIST%" (
    for /r "%~dp0src" %%F in (*.c) do echo "%%F"
)

"%GCC%" -std=c99 -Wall -Wextra -O2 @"%SOURCE_LIST%" ^
    -I"%~dp0src" ^
    -I"%RAYLIB_BUILD%\include" ^
    -L"%RAYLIB_BUILD%" -lraylib -lopengl32 -lgdi32 -lwinmm ^
    -o "%~dp0bin\melting_run_gpu.exe"

set "BUILD_RESULT=%ERRORLEVEL%"
del "%SOURCE_LIST%" >nul 2>nul
if not "%BUILD_RESULT%"=="0" exit /b %BUILD_RESULT%
echo Build completata: bin\melting_run_gpu.exe
