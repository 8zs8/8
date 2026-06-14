================================================================

  抽人软件 - 编译与使用说明

==================================================================


【编译步骤 - Dev-C++】
==================================================================

  1. 打开 Dev-C++
  2. File -> Open Project or File... -> 选择 App.dev
  3. 菜单: Project -> Project Options... -> 点击 Parameters 标签
  4. 在 "Linker" 框中粘贴以下内容:

     -mwindows -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

     重要: -mwindows 必须有，它阻止运行时弹出黑色控制台窗口。

  5. 点 OK，按 Ctrl+F9 编译 App.exe
  6. 关闭这个项目 (File -> Close Project or File)
  7. File -> Open Project or File... -> 选择 Installer.dev
  8. 菜单: Project -> Project Options... -> Parameters 标签
  9. 在 "Linker" 框中粘贴:

     -mwindows -static-libgcc -static-libstdc++ -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

  10. 点 OK，按 Ctrl+F9 编译 Installer.exe


【编译步骤 - 命令行（备用）】
==================================================================

  1. 打开 CMD
  2. 进入源码目录: cd /d E:\802

  3. 编译资源:

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o app_res.o app.rc

  4. 编译 App.exe:

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o App.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

  5. 编译 installer 资源:

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o inst_res.o installer.rc

  6. 编译 Installer.exe:

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o Installer.exe installer.cpp inst_res.o -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

  7. 目录中会有 App.exe 和 Installer.exe


【使用方法】
==================================================================

  方法 A: 直接使用 App.exe
  - 双击 App.exe
  - 桌面右下角出现一个透明的、只有图标的小按钮
  - 单击: 打开 https://8zs8.github.io/8/
  - 长按（按住0.5秒以上）: 弹出右键菜单
  - 拖动: 移动位置（不会打开网页）

  方法 B: 使用 Installer.exe
  - 双击 Installer.exe
  - 点"快速安装"按钮
  - 程序会自动安装到系统，并创建桌面快捷方式和开机自启


【自定义图标】
==================================================================

  1. 准备一个 .ico 文件（建议 48x48 或 64x64）
  2. 把它改名为 app.ico，放在源码目录中，替换现有的 app.ico
  3. 重新编译即可


【文件清单】
==================================================================

  app.cpp            - 主程序源代码（悬浮按钮）
  installer.cpp      - 安装程序源代码
  app.rc             - 主程序资源文件（图标 + 清单）
  installer.rc       - 安装程序资源文件（图标 + 清单 + 嵌入 app.exe）
  manifest.xml       - Windows 程序清单（Common Controls v6）
  app.ico            - 程序图标（可替换）
  App.dev            - Dev-C++ 主程序项目文件
  Installer.dev      - Dev-C++ 安装程序项目文件
  README.txt         - 本说明文件


【常见问题】
==================================================================

  Q: 双击 App.exe 看到黑色控制台窗口一闪而过？
  A: 说明编译时没有加上 -mwindows 参数。回到 Project Options ->
     Parameters -> Linker，确认第一行就是 -mwindows。

  Q: 编译错误: "undefined reference to _imp__..."
  A: Linker 框中缺少 -lxxx 参数。按照【编译步骤】第4或9步复制完整参数。

  Q: 编译错误: "g++.exe: No such file or directory"
  A: Dev-C++ 编译器路径配置错误。菜单: Tools -> Compiler Options ->
     Programs，把 gcc/g++ 路径设置为:
     C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe

  Q: 图标的背景是不透明的方块？
  A: 请使用带透明通道的 .ico 文件。普通 BMP 转换来的 ico 文件
     没有透明通道，会显示为黑色/白色方块背景。
     推荐使用在线工具把 PNG 转 ICO，或使用 IconWorkshop 等专业工具。
