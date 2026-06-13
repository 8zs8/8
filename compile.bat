@echo off
REM ============================================================
REM  Quick Web Launcher Build Script
REM  - ASCII-only (works on any Windows codepage)
REM  - Locates g++.exe by scanning common Dev-C++ / MinGW paths
REM  - Uses absolute paths, NOT dependent on system PATH
REM  Usage: put app.ico in this directory, then double-click
REM ============================================================

setlocal

set LIBS=-luser32 -lgdi32 -lmsimg32 -lshell32 -lshlwapi -lole32 -luuid -lcomctl32 -lcomdlg32
set CFLAGS=-O2 -mwindows -static-libgcc -static-libstdc++

echo ============================================================
echo   Quick Web Launcher - Building
echo ============================================================
echo.

REM --- Locate g++.exe / windres.exe by common paths ---
set GPP=
set WRES=

REM Try system PATH first (fastest)
for /f "delims=" %%i in ('where g++ 2^>nul') do set "GPP=%%i"
for /f "delims=" %%i in ('where windres 2^>nul') do set "WRES=%%i"

if "%GPP%"=="" (
    REM Try Dev-C++ default install paths
    if exist "C:\Dev-Cpp\MinGW64\bin\g++.exe"       set "GPP=C:\Dev-Cpp\MinGW64\bin\g++.exe"
    if exist "C:\Dev-Cpp\MinGW64\bin\windres.exe"    set "WRES=C:\Dev-Cpp\MinGW64\bin\windres.exe"
)
if "%GPP%"=="" (
    if exist "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe"  set "GPP=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe"
    if exist "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" set "WRES=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe"
)
if "%GPP%"=="" (
    if exist "C:\Program Files\Dev-Cpp\MinGW64\bin\g++.exe"   set "GPP=C:\Program Files\Dev-Cpp\MinGW64\bin\g++.exe"
    if exist "C:\Program Files\Dev-Cpp\MinGW64\bin\windres.exe" set "WRES=C:\Program Files\Dev-Cpp\MinGW64\bin\windres.exe"
)
if "%GPP%"=="" (
    REM Try D and E drives (common non-system install locations)
    if exist "D:\Dev-Cpp\MinGW64\bin\g++.exe"       set "GPP=D:\Dev-Cpp\MinGW64\bin\g++.exe"
    if exist "D:\Dev-Cpp\MinGW64\bin\windres.exe"    set "WRES=D:\Dev-Cpp\MinGW64\bin\windres.exe"
)
if "%GPP%"=="" (
    if exist "E:\Dev-Cpp\MinGW64\bin\g++.exe"       set "GPP=E:\Dev-Cpp\MinGW64\bin\g++.exe"
    if exist "E:\Dev-Cpp\MinGW64\bin\windres.exe"    set "WRES=E:\Dev-Cpp\MinGW64\bin\windres.exe"
)
if "%GPP%"=="" (
    REM MinGW-w64 standalone (from SourceForge / MSYS2 offline)
    if exist "C:\mingw64\bin\g++.exe"               set "GPP=C:\mingw64\bin\g++.exe"
    if exist "C:\mingw64\bin\windres.exe"            set "WRES=C:\mingw64\bin\windres.exe"
)
if "%GPP%"=="" (
    if exist "C:\TDM-GCC-64\bin\g++.exe"            set "GPP=C:\TDM-GCC-64\bin\g++.exe"
    if exist "C:\TDM-GCC-64\bin\windres.exe"         set "WRES=C:\TDM-GCC-64\bin\windres.exe"
)
if "%GPP%"=="" (
    if exist "C:\TDM-GCC\bin\g++.exe"              set "GPP=C:\TDM-GCC\bin\g++.exe"
    if exist "C:\TDM-GCC\bin\windres.exe"           set "WRES=C:\TDM-GCC\bin\windres.exe"
)

REM If windres still unknown but g++ found, derive windres from same dir
if not "%GPP%"=="" if "%WRES%"=="" (
    for %%F in ("%GPP%") do set "BINDIR=%%~dpF"
    if exist "%BINDIR%windres.exe" set "WRES=%BINDIR%windres.exe"
)

if "%GPP%"=="" (
    echo [ERROR] Cannot find g++.exe
    echo.
    echo         It looks like your Dev-C++ is installed in a location I didn't scan.
    echo         Please tell me the FULL PATH of g++.exe.
    echo         (e.g. C:\Dev-Cpp\MinGW64\bin\g++.exe)
    echo.
    echo         Or you can edit this compile.bat:
    echo           - find the line "set GPP=" near the top
    echo           - change it to: set GPP=your-actual-path-to-g++.exe
    echo.
    pause
    exit /b 1
)

echo [INFO] Found g++     : %GPP%
echo [INFO] Found windres : %WRES%
echo.

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
