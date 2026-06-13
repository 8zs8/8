@echo off
REM =====================================================================
REM  通用编译脚本 —— 适用于 Dev-C++ / Code::Blocks / 任何自带 GCC 的 C++ IDE
REM
REM  功能：仅依赖 g++ 和 windres（你的 IDE 已经自带）
REM        自动按正确顺序编译：主程序 → 把主程序作为资源嵌入安装程序
REM
REM  使用方法：
REM     1) 把你的图标文件命名为 app.ico，放在本目录
REM     2) 双击本脚本
REM     3) 编译完成后，同目录会出现 app.exe 和 installer.exe
REM
REM  常见问题：
REM     • 报错 'g++' 不是内部或外部命令
REM         → 打开 Dev-C++ 菜单：工具 → 编译选项 → 目录
REM           复制编译器 Bin 目录（形如 C:\Dev-Cpp\MinGW64\bin），
REM           然后在本目录下新建一个 setpath.bat，写：
REM                set PATH=C:\Dev-Cpp\MinGW64\bin;%%PATH%%
REM                cmd /k
REM           然后双击 setpath.bat，在弹出的命令行里再运行 compile.bat
REM     • 报错 windres: can't open `app.ico'
REM         → 本目录下缺少 app.ico，请把图标文件改名为 app.ico
REM =====================================================================

setlocal enabledelayedexpansion

echo ========================================================
echo   Quick Web Launcher  —— 通用编译脚本
echo   依赖：g++ 与 windres（Dev-C++/Code::Blocks 自带）
echo ========================================================
echo.

REM ---------- 检测编译器是否可用 ----------
where g++     >nul 2>&1
if errorlevel 1 goto COMPILER_NOT_FOUND

where windres >nul 2>&1
if errorlevel 1 goto COMPILER_NOT_FOUND

REM ---------- 检测图标文件 ----------
if not exist "app.ico" (
    echo [错误] 本目录下未找到 app.ico
    echo        请把自定义图标命名为 app.ico 后重新运行。
    echo.
    pause
    exit /b 1
)

REM ============================================================
REM  阶段 1：编译主程序 app.exe
REM ============================================================
echo [1/4] 编译主程序资源 ...
windres -o app_res.o app.rc
if errorlevel 1 goto ERROR

echo [2/4] 编译链接主程序 app.exe ...
g++ -O2 -std=c++11 -mwindows -static-libgcc -static-libstdc++ ^
    -o app.exe app.cpp app_res.o ^
    -luser32 -lgdi32 -lshell32 -lole32 -luuid -lcomctl32
if errorlevel 1 goto ERROR

if not exist "app.exe" (
    echo [错误] app.exe 编译未成功生成。
    pause
    exit /b 1
)

echo        → app.exe 已生成 OK

REM ============================================================
REM  阶段 2：编译安装程序 installer.exe
REM          关键：installer.rc 会把上一步生成的 app.exe
REM               作为二进制 RCDATA 资源嵌入
REM ============================================================
echo.
echo [3/4] 编译安装程序资源（自动嵌入 app.exe） ...
windres -o inst_res.o installer.rc
if errorlevel 1 goto ERROR

echo [4/4] 编译链接安装程序 installer.exe ...
g++ -O2 -std=c++11 -mwindows -static-libgcc -static-libstdc++ ^
    -o installer.exe installer.cpp inst_res.o ^
    -luser32 -lgdi32 -lshell32 -lole32 -luuid -lcomctl32
if errorlevel 1 goto ERROR

if not exist "installer.exe" (
    echo [错误] installer.exe 编译未成功生成。
    pause
    exit /b 1
)

echo        → installer.exe 已生成 OK

REM ============================================================
REM  清理临时文件并显示结果
REM ============================================================
echo.
echo ============================================================
echo   ✅ 全部编译完成！
echo.
echo   产出文件：
echo     - app.exe        （主程序：托盘 + 置顶悬浮按钮）
echo     - installer.exe  （一键安装程序，分发给用户即可）
echo.
echo   使用方法：
echo     → 把 installer.exe 发给用户，双击后自动：
echo        ① 解压主程序到安装目录
echo        ② 写入注册表（显示在"程序和功能"中可卸载）
echo        ③ 创建桌面快捷方式：抽人软件.lnk
echo        ④ 设置开机自启
echo        ⑤ 启动主程序
echo.
echo   调试建议：
echo     → 直接双击 app.exe 可独立运行，方便你先调试界面
echo.
echo ============================================================
echo.

REM 清理临时 .o 文件
del /q app_res.o inst_res.o 2>nul

pause
endlocal
exit /b 0

REM ========= 错误处理 =========
:COMPILER_NOT_FOUND
echo.
echo [错误] 未找到 g++ 或 windres。
echo.
echo 解决方法（任选其一）：
echo.
echo   1. 打开 Dev-Cpp，菜单： 工具 ^> 编译选项 ^> 目录
echo      复制 Bin 目录的完整路径，比如：C:\Dev-Cpp\MinGW64\bin
echo      然后在本目录新建一个 setpath.bat，内容：
echo          set PATH=C:\Dev-Cpp\MinGW64\bin;%%PATH%%
echo          cmd /k
echo      双击 setpath.bat，在弹出的命令行里再执行 compile.bat
echo.
echo   2. 或者直接在 Dev-Cpp 里打开本目录下的 App.dev 与 Installer.dev
echo      分别编译（先编译 App.dev → 再编译 Installer.dev）
echo.
pause
exit /b 1

:ERROR
echo.
echo [错误] 编译中断。请查看上方的编译器输出。
echo.
echo 常见原因：
echo   • app.ico 缺失 —— 请把自定义图标改名为 app.ico
echo   • 代码里有语法错误 —— 请查看编译器的 error / warning 行
echo   • 缺少 Windows API —— 本项目只使用 Windows 原生 API，
echo                    不应缺少，若报错请检查 IDE 编译器版本
echo.
pause
exit /b 1
