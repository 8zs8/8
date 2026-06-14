=========================================
Quick Web Launcher - 编译说明
=========================================

【环境要求】
- Windows系统
- Dev-C++ 已安装（包含MinGW64编译器）
  编译器路径：C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe

=========================================
【最简单的编译方法】
=========================================

只需要双击：build.bat

- 自动编译 app.exe 和 installer.exe
- 无需任何配置
- 完成后按任意键退出

=========================================
【其他编译方法】
=========================================

方法二：使用 mingw32-make

  1. 打开CMD，进入此文件夹
  2. 运行：mingw32-make
  3. 或运行：mingw32-make -f Makefile

方法三：使用 Dev-C++

  1. 双击 App.dev 打开工程
  2. 按 Ctrl+F9 编译
  3. 双击 Installer.dev 打开工程
  4. 按 Ctrl+F9 编译

  注意：如果 Dev-C++ 报错链接错误，请使用方法一（build.bat）

方法四：手动命令行

  app.exe:
    windres -o app_res.o app.rc
    g++ -mwindows -O2 -static-libgcc -static-libstdc++ -o app.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

  installer.exe:
    windres -o inst_res.o installer.rc
    g++ -mwindows -O2 -static-libgcc -static-libstdc++ -o installer.exe installer.cpp inst_res.o -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

=========================================
【自定义图标】
=========================================

将你的图标文件命名为 app.ico，放在此文件夹中。
如果没有 app.ico，程序将使用系统默认图标。

=========================================
【生成的文件】
=========================================

- app.exe - 主程序（右下角悬浮按钮，点击打开网页）
- installer.exe - 安装程序（安装后会创建快捷方式和开机自启）

运行 installer.exe 即可安装程序。

=========================================
【常见问题】
=========================================

Q: 编译报错 "undefined reference to..."
A: 使用 build.bat 编译，它包含了所有必要的链接库参数。

Q: Dev-C++ 打开工程文件报错
A: 直接双击 build.bat，它不依赖 Dev-C++ 工程文件。

Q: 编译器找不到
A: 确认 Dev-C++ 安装在：C:\Program Files (x86)\Dev-Cpp\
