@echo off
REM ============================================================
REM  Build script for Quick Web Launcher - APP only
REM  Run this FIRST, then run build-installer.bat
REM ============================================================

setlocal

echo ============================================================
echo   Quick Web Launcher Build Script - App
echo ============================================================
echo.

REM --- Path to Dev-C++ MinGW compiler ---
set GPP=C:\Dev-Cpp\MinGW64\bin\g++.exe
set WRES=C:\Dev-Cpp\MinGW64\bin\windres.exe

REM --- Verify tools exist ---
if not exist "%GPP%" (
    echo [ERROR] Cannot find g++ at: %GPP%
    echo   Please edit this file and update the GPP= line above.
    pause
    exit /b 1
)
if not exist "%WRES%" (
    echo [ERROR] Cannot find windres at: %WRES%
    echo   Please edit this file and update the WRES= line above.
    pause
    exit /b 1
)
if not exist "app.ico" (
    echo [ERROR] app.ico not found. Place your icon in this folder.
    pause
    exit /b 1
)

echo [OK] Tools and icon found
echo.

REM --- Step 1: compile app.rc ---
echo [Step 1] Compiling resource: app.rc
"%WRES%" -o app_res.o app.rc
if errorlevel 1 (
    echo [FAIL] windres failed on app.rc
    pause
    exit /b 1
)
echo        app_res.o created OK
echo.

REM --- Step 2: link app.exe ---
REM NOTE: -l flags MUST come AFTER app.cpp and app_res.o
echo [Step 2] Linking app.exe...
"%GPP%" -O2 -mwindows -static-libgcc -static-libstdc++ ^
    -o app.exe app.cpp app_res.o ^
    -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

if errorlevel 1 (
    echo [FAIL] Linker failed for app.exe
    pause
    exit /b 1
)

if not exist "app.exe" (
    echo [FAIL] app.exe was not created
    pause
    exit /b 1
)

echo.
echo ============================================================
echo   app.exe built successfully.
echo.
echo   Next step: run build-installer.bat to build installer.exe
echo ============================================================
echo.

REM cleanup
del /q app_res.o 2>nul

pause
endlocal
