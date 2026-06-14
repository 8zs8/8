@echo off
REM =====================================================================
REM  Quick Web Launcher - Build Installer (the simplest possible)
REM
REM  Prerequisite: app.exe must exist in the same directory
REM                (run build-app.bat first).
REM =====================================================================

setlocal

echo.
echo  =======================================================
echo   Quick Web Launcher - Building installer.exe
echo  =======================================================
echo.

where g++     >nul 2>&1 || goto :no_tool
where windres >nul 2>&1 || goto :no_tool

if not exist app.exe (
    echo  [ERROR] app.exe not found.
    echo          You must build app.exe first. Run build-app.bat.
    echo.
    pause
    exit /b 1
)

echo  [1/2] windres -o inst_res.o installer.rc
windres -o inst_res.o installer.rc
if errorlevel 1 (
    echo.
    echo  [FAIL] windres failed. See error above.
    pause
    exit /b 1
)

echo  [2/2] g++ -O2 -mwindows -o installer.exe installer.cpp inst_res.o ^
echo         -luser32 -lgdi32 -lmsimg32 -lshell32 -lshlwapi ^
echo         -lole32 -luuid -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

g++ -O2 -mwindows -static-libgcc -static-libstdc++ -o installer.exe ^
    installer.cpp inst_res.o ^
    -luser32 -lgdi32 -lmsimg32 -lshell32 -lshlwapi ^
    -lole32 -luuid -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

if errorlevel 1 (
    echo.
    echo  [FAIL] g++ failed. See error above.
    pause
    exit /b 1
)

if not exist installer.exe (
    echo  [FAIL] installer.exe was not created.
    pause
    exit /b 1
)

del /q inst_res.o 2>nul

echo.
echo  =======================================================
echo   installer.exe built successfully.
echo   This is the file you distribute to users.
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
