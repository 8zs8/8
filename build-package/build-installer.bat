@echo off
REM ====================================================================
REM  Step 2 of 2: Build installer.exe
REM  Just double-click this file.
REM ====================================================================

setlocal

echo.
echo  ========================================================
echo   Step 2/2: Building installer.exe
echo  ========================================================
echo.

set MINGW=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin
set GCC=%MINGW%\g++.exe
set WRC=%MINGW%\windres.exe

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

if not exist app.exe (
    echo  [ERROR] app.exe not found.
    echo.
    echo  Run build-app.bat FIRST to build app.exe.
    echo  Then run this file again.
    echo.
    pause
    exit /b 1
)

echo  [1/2] Compiling resource (installer.rc -^> inst_res.o)...
"%WRC%" -o inst_res.o installer.rc
if errorlevel 1 (
    echo  [FAIL] windres failed.
    pause
    exit /b 1
)

echo  [2/2] Linking (installer.cpp + inst_res.o -^> installer.exe)...
"%GCC%" -mwindows -O2 -static-libgcc -static-libstdc++ ^
    -o installer.exe installer.cpp inst_res.o ^
    -lshell32 -lshlwapi -lole32 -luuid ^
    -luser32 -lgdi32 -lmsimg32 ^
    -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32
if errorlevel 1 (
    echo  [FAIL] g++ failed.
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
echo  ========================================================
echo   installer.exe built successfully!
echo   You now have: app.exe and installer.exe
echo  ========================================================
echo.
pause
exit /b 0
