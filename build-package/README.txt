===============================================================
  抽人软件 - 编译说明
===============================================================

不再有控制台黑窗！
#pragma comment(linker, "-mwindows") 已写入源代码，
链接器会自动将程序标记为 GUI 子系统。

===============================================================
【编译步骤】
===============================================================

1. 打开 Dev-C++
2. File -> Open -> 选择 App.dev
3. 按 Ctrl+F9 编译 App.exe
4. File -> Close Project -> File -> Open -> Installer.dev
5. 按 Ctrl+F9 编译 Installer.exe

如出现链接错误，在 Project -> Project Options ->
Parameters -> Linker 框中检查并粘贴：

  App.exe:
  -mwindows -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

  Installer.exe:
  -mwindows -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

===============================================================
【功能说明】
===============================================================

- 双击 App.exe：右下角出现悬浮按钮
- 单击按钮：打开网页
- 拖动按钮：移动位置
- 右键按钮：弹出菜单（打开网页 / 退出）
- 托盘图标：同上
- 无控制台黑窗