@echo off
REM ====================================================================
REM  Step 2 of 2: Build installer.exe
REM  Just double-click this file and it will do everything.
REM ====================================================================

echo.
echo  ========================================================
echo   Step 2/2: Building installer.exe
echo  ========================================================
echo.

REM Add Dev-C++ MinGW to PATH
set PATH=C:\Dev-Cpp\MinGW64\bin;%PATH%

REM Verify tools exist
where g++     >nul 2>&1 || goto :no_g++
where windres  >nul 2>&1 || goto :no_windres

REM Verify app.exe exists (must be built first)
if not exist app.exe (
    echo  [ERROR] app.exe not found.
    echo.
    echo  You must run build-app.bat FIRST to build app.exe.
    echo  Then run this file again.
    echo.
    pause
    exit /b 1
)

REM Compile resource
echo  [1/2] Compiling resource (installer.rc -^> inst_res.o)...
windres -o inst_res.o installer.rc
if errorlevel 1 (
    echo  [FAIL] windres failed.
    pause
    exit /b 1
)

REM Link
echo  [2/2] Linking (installer.cpp + inst_res.o -^> installer.exe)...
g++ -mwindows -O2 -static-libgcc -static-libstdc++ ^
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

REM Clean up temp file
del /q inst_res.o 2>nul

echo.
echo  ========================================================
echo   installer.exe built successfully!
echo   Done. You now have app.exe and installer.exe.
echo  ========================================================
echo.
pause
exit /b 0

:no_g++
echo  [ERROR] g++ not found.
echo.
echo  Your Dev-C++ MinGW bin folder needs to be in PATH.
echo  Open Dev-C++ ^> Tools ^> Compiler Options ^> Directories ^> Binaries
echo  and copy that path, then tell me the full path.
echo.
echo  Or edit this .bat file and change the line:
echo    set PATH=C:\Dev-Cpp\MinGW64\bin;%%PATH%%
echo  to match your actual Dev-C++ installation folder.
echo.
pause
exit /b 1

:no_windres
echo  [ERROR] windres not found. Your Dev-C++ installation may be incomplete.
echo.
pause
exit /b 1
