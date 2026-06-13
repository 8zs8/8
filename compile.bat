@echo off
REM ============================================================
REM  Quick Web Launcher Build Script
REM  - ASCII-only (works on any Windows codepage)
REM  - Uses absolute paths, NOT dependent on system PATH
REM  - Scans common Dev-C++ / MinGW install locations
REM  Usage: put app.ico in this directory, then double-click
REM ============================================================

setlocal

set LIBS=-luser32 -lgdi32 -lmsimg32 -lshell32 -lshlwapi -lole32 -luuid -lcomctl32 -lcomdlg32
set CFLAGS=-O2 -mwindows -static-libgcc -static-libstdc++

echo ============================================================
echo   Quick Web Launcher - Building
echo ============================================================
echo.

REM --- Locate g++.exe / windres.exe ---
set GPP=
set WRES=

REM Check the most common paths FIRST (highest priority -> lowest)
call :CheckPath "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin"
if "%GPP%"=="" call :CheckPath "C:\Program Files\Dev-Cpp\MinGW64\bin"
if "%GPP%"=="" call :CheckPath "C:\Dev-Cpp\MinGW64\bin"
if "%GPP%"=="" call :CheckPath "D:\Dev-Cpp\MinGW64\bin"
if "%GPP%"=="" call :CheckPath "E:\Dev-Cpp\MinGW64\bin"
if "%GPP%"=="" call :CheckPath "E:\Dev-Cpp\MinGW64\bin"
if "%GPP%"=="" call :CheckPath "C:\mingw64\bin"
if "%GPP%"=="" call :CheckPath "C:\TDM-GCC-64\bin"
if "%GPP%"=="" call :CheckPath "C:\TDM-GCC\bin"

REM As final fallback, try via system PATH (for users who already set it)
if "%GPP%"=="" (
    for %%X in (g++.exe) do (
        if not "%%~$PATH:X"=="" set "GPP=%%~$PATH:X"
    )
    for %%X in (windres.exe) do (
        if not "%%~$PATH:X"=="" set "WRES=%%~$PATH:X"
    )
)

if "%GPP%"=="" (
    echo [ERROR] Cannot find g++.exe
    echo.
    echo         Please edit this compile.bat and add these two lines
    echo         right after the "setlocal" line at the top:
    echo.
    echo             set GPP=YOUR-PATH-TO\g++.exe
    echo             set WRES=YOUR-PATH-TO\windres.exe
    echo.
    echo         (Replace YOUR-PATH-TO with the actual directory path,
    echo          e.g. C:\Program Files (x86)\Dev-Cpp\MinGW64\bin)
    echo.
    pause
    exit /b 1
)

echo [INFO] Using g++     : %GPP%
echo [INFO] Using windres : %WRES%
echo.

REM --- Check icon ---
if not exist "app.ico" (
    echo [ERROR] app.ico is missing. Put your custom icon in this
    echo         directory with the name: app.ico
    echo.
    pause
    exit /b 1
)

REM ==================== MAIN PROGRAM ========================
echo [1/4] Compiling resource: app_res.o ...
"%WRES%" -o app_res.o app.rc
if errorlevel 1 (
    echo [FAIL] Resource compiler failed.
    pause
    exit /b 1
)

echo [2/4] Building app.exe ...
"%GPP%" %CFLAGS% -o app.exe app.cpp app_res.o %LIBS%
if errorlevel 1 (
    echo [FAIL] Cannot link app.exe
    pause
    exit /b 1
)
echo        OK - app.exe built
echo.

REM =================== INSTALLER ============================
echo [3/4] Compiling installer resource ...
"%WRES%" -o inst_res.o installer.rc
if errorlevel 1 (
    echo [FAIL] installer.rc compile failed. Make sure app.exe exists.
    pause
    exit /b 1
)

echo [4/4] Building installer.exe ...
"%GPP%" %CFLAGS% -o installer.exe installer.cpp inst_res.o %LIBS%
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
exit /b 0

REM ---------- helper subroutine ----------
:CheckPath
    if exist "%~1\g++.exe" (
        set "GPP=%~1\g++.exe"
        if exist "%~1\windres.exe" set "WRES=%~1\windres.exe"
    )
exit /b
