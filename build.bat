@echo off
REM =========================================================
REM  构建脚本（需要 MinGW 环境，如 TDM-GCC 或 MSYS2/MinGW-w64）
REM
REM  关键构建顺序：
REM    1) 编译主程序 app.exe（嵌入 app.ico 资源）
REM    2) 把 app.exe 作为二进制资源（RCDATA 1001/1002）
REM       嵌入 installer.exe。因此最终仅需分发 installer.exe
REM       一个文件给用户即可。
REM
REM  准备工作：请将你的图标文件命名为 app.ico 放在本目录下。
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
g++ -O2 -mwindows -static-libgcc -static-libstdc++ ^
    -o app.exe app.cpp app_res.o ^
    -lshell32 -lshlwapi -luser32 -lgdi32 -lole32 -luuid
if errorlevel 1 (
    echo app.exe 构建失败
    pause
    exit /b 1
)

echo.
echo [3/4] 编译安装程序资源（此时会把 app.exe 作为二进制资源嵌入） ...
windres -o inst_res.o installer.rc
if errorlevel 1 (
    echo 资源编译失败
    pause
    exit /b 1
)

echo.
echo [4/4] 编译并链接安装程序 installer.exe ...
g++ -O2 -mwindows -static-libgcc -static-libstdc++ ^
    -o installer.exe installer.cpp inst_res.o ^
    -lshell32 -lshlwapi -luser32 -lgdi32 -lole32 -luuid
if errorlevel 1 (
    echo installer.exe 构建失败
    pause
    exit /b 1
)

echo.
echo ==========================================================
echo   构建完成！
echo   - app.exe        （主程序：托盘 + 悬浮置顶按钮，已嵌入到 installer）
echo   - installer.exe  （安装程序：一键安装 —— 只需分发这个文件给用户）
echo
echo   安装流程：解压主程序 → 注册组件（注册表/快捷方式/自启动）
echo   桌面快捷方式名：抽人软件.lnk
echo ==========================================================
echo.

del /q app_res.o inst_res.o 2>nul

pause
endlocal
