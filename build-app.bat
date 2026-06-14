@echo off
REM ====================================================================
REM  Step 1 of 2: Build app.exe
REM  Just double-click this file and it will do everything.
REM ====================================================================

echo.
echo  ========================================================
echo   Step 1/2: Building app.exe
echo  ========================================================
echo.

REM Add Dev-C++ MinGW to PATH (adjust if yours is somewhere else)
set PATH=C:\Dev-Cpp\MinGW64\bin;%PATH%

REM Verify tools exist
where g++     >nul 2>&1 || goto :no_g++
where windres  >nul 2>&1 || goto :no_windres

REM Verify icon exists
if not exist app.ico (
    echo  [ERROR] app.ico not found. Put it in the same folder as this file.
    pause
    exit /b 1
)

REM Compile resource
echo  [1/2] Compiling resource (app.rc -^> app_res.o)...
windres -o app_res.o app.rc
if errorlevel 1 (
    echo  [FAIL] windres failed.
    pause
    exit /b 1
)

REM Link
echo  [2/2] Linking (app.cpp + app_res.o -^> app.exe)...
g++ -mwindows -O2 -static-libgcc -static-libstdc++ ^
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

REM Clean up temp file
del /q app_res.o 2>nul

echo.
echo  ========================================================
echo   app.exe built successfully!
echo   Next: double-click  build-installer.bat
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
