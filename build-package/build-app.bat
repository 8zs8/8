@echo off
REM ====================================================================
REM  Step 1 of 2: Build app.exe
REM  Just double-click this file.
REM ====================================================================

setlocal

echo.
echo  ========================================================
echo   Step 1/2: Building app.exe
echo  ========================================================
echo.

REM The actual MinGW bin folder on your system
set MINGW=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin

REM Use absolute paths - no need for system PATH
set GCC=%MINGW%\g++.exe
set WRC=%MINGW%\windres.exe

REM Verify tools exist
if not exist "%GCC%" (
    echo  [ERROR] g++.exe not found at: %GCC%
    pause
    exit /b 1
)
if not exist "%WRC%" (
    echo  [ERROR] windres.exe not found at: %WRC%
    pause
    exit /b 1
)

REM Verify icon exists (app.ico - replace with your own icon)
if not exist app.ico (
    echo  [WARNING] app.ico not found. Using default system icon.
    echo  To use your own icon, put app.ico in this folder.
    echo.
    REM Write a minimal RC that uses a built-in Windows icon
    echo 1 24 "manifest.xml" > app.rc
    echo // Note: no custom app.ico found, using system default.
)

echo  [1/2] Compiling resource (app.rc -^> app_res.o)...
"%WRC%" -o app_res.o app.rc
if errorlevel 1 (
    echo  [FAIL] windres failed.
    pause
    exit /b 1
)

echo  [2/2] Linking (app.cpp + app_res.o -^> app.exe)...
"%GCC%" -mwindows -O2 -static-libgcc -static-libstdc++ ^
    -o app.exe app.cpp app_res.o ^
    -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
if errorlevel 1 (
    echo  [FAIL] g++ failed.
    pause
    exit /b 1
)

if not exist app.exe (
    echo  [FAIL] app.exe was not created.
    pause
    exit /b 1
)

del /q app_res.o 2>nul

echo.
echo  ========================================================
echo   app.exe built successfully!
echo   Next: double-click  build-installer.bat
echo  ========================================================
echo.
pause
exit /b 0
