===============================================================
  抽人软件 - 编译说明
===============================================================

【编译步骤】
===============================================================

1. 打开 Dev-C++
2. File -> Open -> 选择 App.dev
3. 按 Ctrl+F9 编译 App.exe
4. File -> Close Project -> File -> Open -> Installer.dev
5. 按 Ctrl+F9 编译 Installer.exe

如果还出现链接错误，手动检查：
Project -> Project Options -> Parameters

App.dev 的两个框：
  Linker 框:    -mwindows -static-libgcc -static-libstdc++
  Libs/库 框:   -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

Installer.dev 的两个框：
  Linker 框:    -mwindows -static-libgcc -static-libstdc++
  Libs/库 框:   -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

===============================================================
【功能说明】
===============================================================

- 双击 App.exe：右下角出现 68x68 悬浮按钮
- 单击按钮或点击托盘图标：打开网页
- 拖动按钮：移动位置
- 右键按钮或托盘图标：弹出菜单
- 无控制台黑窗