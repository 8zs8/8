@echo off
REM =====================================================================
REM  直接编译脚本（双击此文件即可编译，不依赖 Dev-C++ 工程解析）
REM  步骤：1. windres 编译 .rc -> .o   2. g++ 编译 .cpp -> .o   3. g++ 链接 .o -> .exe
REM =====================================================================

setlocal

set "GPP=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe"
set "WINDRES=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe"

echo [1/3] 编译资源 app.rc ...
"%WINDRES%" -o app_res.o app.rc
if errorlevel 1 (
    echo FAIL: windres
    pause
    exit /b 1
)
echo OK: app_res.o

echo.
echo [2/3] 编译 app.cpp ...
"%GPP%" -c -O2 -o app.o app.cpp
if errorlevel 1 (
    echo FAIL: compile cpp
    pause
    exit /b 1
)
echo OK: app.o

echo.
echo [3/3] 链接 app.exe ...
"%GPP%" -mwindows -o app.exe app.o app_res.o -lgdi32 -luser32 -lshell32 -lcomctl32 -lkernel32 -static-libgcc -static-libstdc++
if errorlevel 1 (
    echo FAIL: link
    pause
    exit /b 1
)
echo OK: app.exe

echo.
echo ===== 编译完成！双击 app.exe 运行程序 =====
pause
endlocal
