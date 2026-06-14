@echo off
REM ====================================================================
REM  ONE-CLICK BUILD - Quick Web Launcher
REM
REM  Just double-click this file!
REM  No configuration needed - everything is automatic.
REM ====================================================================

setlocal enabledelayedexpansion

echo.
echo  ========================================================
echo   Quick Web Launcher - Building...
echo  ========================================================
echo.

REM Compiler location (hardcoded - DO NOT CHANGE)
set "GCC=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe"
set "WRC=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe"

REM Verify compiler exists
if not exist "%GCC%" (
    echo  ERROR: g++.exe not found at:
    echo  %GCC%
    echo.
    echo  Please make sure Dev-C++ is installed correctly.
    echo  Press any key to exit...
    pause >nul
    exit /b 1
)

REM ====================================================================
REM STEP 1: Build app.exe
REM ====================================================================
echo.
echo  [1/3] Checking resources...

REM Check if app.ico exists
if exist app.ico (
    echo  [OK] app.ico found - will use custom icon
) else (
    echo  [OK] No app.ico - will use default icon
)

echo.
echo  [2/3] Compiling app.rc...
"%WRC%" -o app_res.o app.rc
if errorlevel 1 (
    echo  FAILED: Could not compile app.rc
    echo  Check that app.rc and manifest.xml exist.
    pause
    exit /b 1
)

echo.
echo  [3/3] Linking app.exe...
echo  Linker command:
echo  "%GCC%" -mwindows -O2 -static-libgcc -static-libstdc++ -o app.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
echo.

"%GCC%" -mwindows -O2 -static-libgcc -static-libstdc++ -o app.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
if errorlevel 1 (
    echo  FAILED: Could not link app.exe
    echo  Check error messages above.
    del /q app_res.o 2>nul
    pause
    exit /b 1
)

if not exist app.exe (
    echo  FAILED: app.exe was not created!
    del /q app_res.o 2>nul
    pause
    exit /b 1
)

echo.
echo  [OK] app.exe built successfully!

REM Clean up temp file
del /q app_res.o 2>nul

REM ====================================================================
REM STEP 2: Build installer.exe
REM ====================================================================
echo.
echo  [4/5] Compiling installer.rc...
"%WRC%" -o inst_res.o installer.rc
if errorlevel 1 (
    echo  FAILED: Could not compile installer.rc
    echo  Check that installer.rc and manifest.xml exist.
    pause
    exit /b 1
)

echo.
echo  [5/5] Linking installer.exe...
echo  Linker command:
echo  "%GCC%" -mwindows -O2 -static-libgcc -static-libstdc++ -o installer.exe installer.cpp inst_res.o -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32
echo.

"%GCC%" -mwindows -O2 -static-libgcc -static-libstdc++ -o installer.exe installer.cpp inst_res.o -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32
if errorlevel 1 (
    echo  FAILED: Could not link installer.exe
    echo  Check error messages above.
    del /q inst_res.o 2>nul
    pause
    exit /b 1
)

if not exist installer.exe (
    echo  FAILED: installer.exe was not created!
    del /q inst_res.o 2>nul
    pause
    exit /b 1
)

REM Clean up temp files
del /q inst_res.o 2>nul

REM ====================================================================
REM SUCCESS!
REM ====================================================================
echo.
echo  ========================================================
echo   BUILD SUCCESSFUL!
echo  ========================================================
echo.
echo   Files created:
echo   - app.exe       (main program)
echo   - installer.exe (installer)
echo.
echo   Run installer.exe to install the program.
echo.
echo  ========================================================
echo.

pause
