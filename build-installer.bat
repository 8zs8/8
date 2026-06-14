@echo off
REM ============================================================
REM  Build script for Quick Web Launcher
REM  - Fully manual: copy-paste each line into your Dev-C++
REM    Terminal OR double-click this .bat
REM  - Uses explicit absolute paths to Dev-C++ MinGW bin
REM  - Library flags come AFTER object files (required by GCC/ld)
REM ============================================================

setlocal

echo ============================================================
echo   Quick Web Launcher Build Script
echo ============================================================
echo.

REM --- Path to Dev-C++ MinGW compiler (adjust if yours is different) ---
set GPP=C:\Dev-Cpp\MinGW64\bin\g++.exe
set WRES=C:\Dev-Cpp\MinGW64\bin\windres.exe

REM --- Verify tools exist ---
if not exist "%GPP%" (
    echo [ERROR] Cannot find g++ at:
    echo         %GPP%
    echo.
    echo   Please edit this file and update the GPP= line above.
    echo   Or check where your Dev-Cpp\MinGW64\bin\g++.exe actually is.
    echo.
    pause
    exit /b 1
)
if not exist "%WRES%" (
    echo [ERROR] Cannot find windres at:
    echo         %WRES%
    echo.
    echo   Please edit this file and update the WRES= line above.
    echo.
    pause
    exit /b 1
)
if not exist "app.ico" (
    echo [ERROR] app.ico not found in current directory.
    echo         Please place your custom icon file (named app.ico)
    echo         in the same folder as this build.bat
    echo.
    pause
    exit /b 1
)
if not exist "app.exe" (
    echo [ERROR] app.exe not found.
    echo         You must build app.exe BEFORE installer.exe.
    echo         Run this script TWICE:
    echo           First run:  builds app.exe
    echo           Second run: builds installer.exe (embeds app.exe)
    echo.
    pause
    exit /b 1
)

echo [OK] All tools and app.exe found.
echo.

REM --- Step 1: compile installer.rc ---
echo [Step 1] Compiling installer resource...
"%WRES%" -o inst_res.o installer.rc
if errorlevel 1 (
    echo [FAIL] windres failed on installer.rc
    pause
    exit /b 1
)
echo        inst_res.o created OK
echo.

REM --- Step 2: link installer.exe ---
REM NOTE: -l flags MUST come AFTER installer.cpp and inst_res.o
echo [Step 2] Linking installer.exe...
"%GPP%" -O2 -mwindows -static-libgcc -static-libstdc++ ^
    -o installer.exe installer.cpp inst_res.o ^
    -luser32 -lgdi32 -lmsimg32 -lshell32 -lshlwapi ^
    -lole32 -luuid -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

if errorlevel 1 (
    echo [FAIL] Linker failed for installer.exe
    pause
    exit /b 1
)

if not exist "installer.exe" (
    echo [FAIL] installer.exe was not created
    pause
    exit /b 1
)

echo.
echo ============================================================
echo   Build completed successfully.
echo.
echo   app.exe       : OK
echo   installer.exe : OK
echo.
echo   Run installer.exe on any Windows machine to install.
echo ============================================================
echo.

REM cleanup intermediate files
del /q inst_res.o 2>nul

pause
endlocal
