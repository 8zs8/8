=========================================
Quick Web Launcher - 编译说明
=========================================

【环境要求】
- Windows系统
- Dev-C++（已安装 MinGW-w64）
  g++ 实际路径：C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe

【编译步骤】

方法一：使用批处理文件（推荐，最简单）

  步骤1：双击运行 build-app.bat
         - 这会编译生成 app.exe（主程序）

  步骤2：双击运行 build-installer.bat
         - 这会编译生成 installer.exe（安装程序）
         - 注意：必须先有 app.exe 才能运行这一步

  步骤3：完成！
         - 现在你有 app.exe 和 installer.exe 两个文件
         - 双击 installer.exe 即可安装程序

方法二：使用 Dev-C++ 打开工程文件

  步骤1：双击 App.dev
         - Dev-C++ 会打开主程序工程
         - 按 Ctrl+F9 编译
         - 会生成 app.exe

  步骤2：双击 Installer.dev
         - Dev-C++ 会打开安装程序工程
         - 按 Ctrl+F9 编译
         - 会生成 installer.exe

【自定义图标】
- 将你自己的图标文件命名为 app.ico 放在此文件夹中
- 如果不提供 app.ico，程序将使用系统默认图标

【程序功能】
- app.exe：右下角置顶悬浮按钮，点击打开指定网页
- installer.exe：安装程序，自动解压、创建快捷方式、开机自启
