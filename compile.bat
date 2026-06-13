@echo off
REM ============================================================
REM  Quick Web Launcher Build Script
REM  All text is ASCII-only so this works on any Windows codepage.
REM
REM  Usage: put app.ico in the same directory, then double-click.
REM  If g++ is not found, run:  set PATH=C:\Dev-Cpp\MinGW64\bin;%PATH%
REM ============================================================

setlocal

set LIBS=-luser32 -lgdi32 -lmsimg32 -lshell32 -lshlwapi -lole32 -luuid -lcomctl32 -lcomdlg32
set CFLAGS=-O2 -mwindows -static-libgcc -static-libstdc++

echo ============================================================
echo   Quick Web Launcher - Building
echo ============================================================
echo.

REM --- Check compiler ---
where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] g++ not found. Make sure Dev-C++ MinGW bin is in PATH.
    echo         Quick fix: set PATH=C:\Dev-Cpp\MinGW64\bin;%%PATH%%
    echo.
    pause
    exit /b 1
)

REM --- Check icon ---
if not exist "app.ico" (
    echo [ERROR] app.ico is missing. Put your custom icon in this directory
    echo         with the name: app.ico
    echo.
    pause
    exit /b 1
)

REM ==================== MAIN PROGRAM ========================
echo [1/4] Compiling resource: app_res.o ...
windres -o app_res.o app.rc
if errorlevel 1 (
    echo [FAIL] Resource compiler failed.
    pause
    exit /b 1
)

echo [2/4] Building app.exe ...
g++ %CFLAGS% -o app.exe app.cpp app_res.o %LIBS%
if errorlevel 1 (
    echo [FAIL] Cannot link app.exe
    pause
    exit /b 1
)
echo        OK - app.exe built
echo.

REM =================== INSTALLER ============================
echo [3/4] Compiling installer resource ...
windres -o inst_res.o installer.rc
if errorlevel 1 (
    echo [FAIL] installer.rc compile failed. Make sure app.exe exists.
    pause
    exit /b 1
)

echo [4/4] Building installer.exe ...
g++ %CFLAGS% -o installer.exe installer.cpp inst_res.o %LIBS%
if errorlevel 1 (
    echo [FAIL] Cannot link installer.exe
    pause
    exit /b 1
)
echo        OK - installer.exe built
echo.

echo ============================================================
echo   Build completed successfully.
echo.
echo   Output files:
echo     app.exe        - tray + top-floating button launcher
echo     installer.exe  - one-click installer (distribute this)
echo.
echo   Test: double-click app.exe to try the main program directly
echo ============================================================
echo.

REM cleanup
del /q app_res.o inst_res.o 2>nul

pause
endlocal
