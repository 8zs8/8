@echo off
chcp 65001 >nul
REM ============================================================
REM  Quick-Web-Launcher 通用编译脚本
REM  说明：所有 -lxxx 库必须放在源文件 / .o 文件之后（GCC/ld 硬性要求）
REM ============================================================

setlocal

set LIBS=-luser32 -lgdi32 -lmsimg32 -lshell32 -lshlwapi -lole32 -luuid -lcomctl32 -lcomdlg32
set CFLAGS=-O2 -mwindows -static-libgcc -static-libstdc++

echo ========================================================
echo   Quick Web Launcher - 编译中
echo ========================================================
echo.

REM 检查编译器
where g++ >nul 2>&1
if errorlevel 1 (
    echo [错误] 未找到 g++. 请确保 Dev-C++ 的 bin 目录在 PATH 中。
    echo        临时方法: set PATH=C:\Dev-Cpp\MinGW64\bin;%%PATH%%
    echo.
    pause
    exit /b 1
)

REM 检查图标
if not exist "app.ico" (
    echo [错误] 同目录下未找到 app.ico, 请把图标放到本目录。
    echo.
    pause
    exit /b 1
)

REM =============== 主程序 app.exe ======================
echo [1/4] 编译资源 app_res.o ...
windres -o app_res.o app.rc
if errorlevel 1 (
    echo [失败] 资源编译失败
    pause
    exit /b 1
)

echo [2/4] 编译 app.exe ...
g++ %CFLAGS% -o app.exe app.cpp app_res.o %LIBS%
if errorlevel 1 (
    echo [失败] app.exe 链接失败
    pause
    exit /b 1
)
echo        OK - app.exe 已生成
echo.

REM =============== 安装程序 installer.exe ===============
echo [3/4] 编译资源 inst_res.o (含 app.exe) ...
windres -o inst_res.o installer.rc
if errorlevel 1 (
    echo [失败] 安装程序资源编译失败
    pause
    exit /b 1
)

echo [4/4] 编译 installer.exe ...
g++ %CFLAGS% -o installer.exe installer.cpp inst_res.o %LIBS%
if errorlevel 1 (
    echo [失败] installer.exe 链接失败
    pause
    exit /b 1
)
echo        OK - installer.exe 已生成
echo.

echo ========================================================
echo   编译成功!
echo.
echo   产物:
echo     app.exe        (托盘 + 置顶悬浮按钮)
echo     installer.exe  (一键安装, 分发给用户)
echo.
echo   直接双击 app.exe 可独立测试主程序
echo   把 installer.exe 发给用户即可安装
echo ========================================================
echo.

REM 清理临时文件
del /q app_res.o inst_res.o 2>nul

pause
endlocal
