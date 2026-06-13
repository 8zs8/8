@echo off
REM =========================================================
REM  构建脚本（需要 MinGW 环境，如 TDM-GCC 或 MSYS2/MinGW-w64）
REM  会产生：app.exe （主程序，后台托盘 + 置顶悬浮按钮）
REM          installer.exe （安装程序）
REM  准备工作：请将你的图标文件命名为 app.ico 放在本目录下
REM =========================================================

setlocal enabledelayedexpansion

where windres >nul 2>&1
if errorlevel 1 (
    echo [错误] 未找到 windres，请先安装 MinGW。
    echo        推荐：MSYS2 安装 mingw-w64-x86_64-gcc，或使用 TDM-GCC
    pause
    exit /b 1
)

where g++ >nul 2>&1
if errorlevel 1 (
    echo [错误] 未找到 g++，请先安装 MinGW。
    pause
    exit /b 1
)

if not exist "app.ico" (
    echo [警告] 当前目录没有 app.ico，将无法显示自定义图标。
    echo        请把你的图标文件命名为 app.ico 放在本目录后重新构建。
    pause
)

echo.
echo [1/4] 编译主程序资源 ...
windres -o app_res.o app.rc
if errorlevel 1 (
    echo 资源编译失败
    pause
    exit /b 1
)

echo.
echo [2/4] 编译并链接主程序 app.exe ...
g++ -O2 -std=c++11 -mwindows -static-libgcc -static-libstdc++ ^
    -o app.exe app.cpp app_res.o ^
    -lshell32 -lshlwapi -luser32 -lgdi32 -lole32 -luuid
if errorlevel 1 (
    echo app.exe 构建失败
    pause
    exit /b 1
)

echo.
echo [3/4] 编译安装程序资源 ...
windres -o inst_res.o installer.rc
if errorlevel 1 (
    echo 资源编译失败
    pause
    exit /b 1
)

echo.
echo [4/4] 编译并链接安装程序 installer.exe ...
g++ -O2 -std=c++11 -mwindows -static-libgcc -static-libstdc++ ^
    -o installer.exe installer.cpp inst_res.o ^
    -lshell32 -lshlwapi -luser32 -lgdi32 -lole32 -luuid
if errorlevel 1 (
    echo installer.exe 构建失败
    pause
    exit /b 1
)

echo.
echo =========================================
echo   构建完成！
echo   - app.exe        （主程序：托盘 + 悬浮置顶按钮）
echo   - installer.exe  （安装程序：一键安装）
echo   请将 app.exe、app.ico、installer.exe
echo   放在同一目录后运行 installer.exe 进行安装。
echo =========================================
echo.

del /q app_res.o inst_res.o 2>nul

pause
endlocal
