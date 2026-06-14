@echo off
REM =====================================================================
REM  Quick Web Launcher - Build App (the simplest possible)
REM
REM  How to use this file:
REM  1. Make sure g++.exe and windres.exe are in your PATH.
REM     - How? In Dev-C++ menu: Tools -> Compiler Options -> Directories -> Binaries
REM       Copy that path (e.g. C:\Dev-Cpp\MinGW64\bin).
REM     - Then in a cmd window run:
REM         set PATH=C:\Dev-Cpp\MinGW64\bin;%PATH%
REM  2. Double-click this file.
REM     - It will NOT try to guess paths and will NOT run if g++ is missing.
REM  3. If something fails, the window stays open so you can read the error.
REM =====================================================================

setlocal

echo.
echo  =======================================================
echo   Quick Web Launcher - Building app.exe
echo  =======================================================
echo.

where g++     >nul 2>&1 || goto :no_tool
where windres >nul 2>&1 || goto :no_tool

if not exist app.ico (
    echo  [ERROR] app.ico not found in current directory.
    echo          Please place your icon in the same folder and retry.
    echo.
    pause
    exit /b 1
)

echo  [1/2] windres -o app_res.o app.rc
windres -o app_res.o app.rc
if errorlevel 1 (
    echo.
    echo  [FAIL] windres failed. See error above.
    pause
    exit /b 1
)

echo  [2/2] g++ -O2 -mwindows -o app.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
g++ -O2 -mwindows -static-libgcc -static-libstdc++ -o app.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
if errorlevel 1 (
    echo.
    echo  [FAIL] g++ failed. See error above.
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
echo  =======================================================
echo   app.exe built successfully.
echo   Next: run build-installer.bat to build installer.exe.
echo  =======================================================
echo.
pause
exit /b 0

:no_tool
echo  [ERROR] g++ or windres not found in PATH.
echo.
echo  Please run this first in the same cmd window:
echo    set PATH=C:\Dev-Cpp\MinGW64\bin;%%PATH%%
echo  (replace C:\Dev-Cpp\MinGW64\bin with your actual path)
echo.
pause
exit /b 1
