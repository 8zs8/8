=========================================
Quick Web Launcher - Dev-C++ 编译指南
=========================================

【环境要求】
- Windows系统
- Dev-C++ (含 MinGW-w64)

【编译步骤 - 方法A：打开工程文件】

1. 在文件管理器中双击 App.dev
2. Dev-C++ 打开后，按 Ctrl+F9 (或菜单: Execute → Compile)
3. 应该会生成 app.exe

【如果出现链接错误 (undefined reference to _imp_...)】

必须通过 Dev-C++ 的图形界面手动添加库：

=== 为 app.exe 配置库 ===

1. 菜单: Project → Project Options...
2. 切换到 "Parameters"（参数）标签页
3. 在 "Linker"（链接器）框中复制并粘贴以下内容（一整行）：

-luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

4. 点击 "OK"
5. 按 Ctrl+F9 编译

=== 为 installer.exe 配置库 ===

1. 在 Dev-C++ 中关闭 App.dev
2. 双击打开 Installer.dev
3. 菜单: Project → Project Options...
4. 切换到 "Parameters" 标签页
5. 在 "Linker" 框中复制并粘贴以下内容（一整行）：

-lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

6. 点击 "OK"
7. 按 Ctrl+F9 编译

【编译步骤 - 方法B：单文件编译】

如果你不想用 .dev 工程文件，也可以：

1. 在 Dev-C++ 菜单: File → New → Project
2. 选择 "Windows Application"，输入项目名，点击 OK
3. 删除默认生成的 main.cpp
4. 菜单: Project → Add to Project，选择 app.cpp 和 app.rc
5. 按上面方法A的步骤3-5配置链接库
6. 按 Ctrl+F9 编译

【库说明】

app.exe 需要的库：
  -luser32    → 窗口管理、消息循环、托盘图标
  -lgdi32     → 图形、圆角区域
  -lmsimg32   → 渐变填充
  -lshell32   → ShellExecuteW (打开URL)
  -lcomctl32  → 通用控件

installer.exe 需要的库：
  -lshell32   → 快捷方式、文件操作
  -lshlwapi   → 路径操作函数
  -lole32     → COM初始化 (CoInitialize/CoCreateInstance)
  -luuid      → IID_IShellLinkW / IID_IPersistFile 等GUID
  -luser32    → 安装界面窗口、控件
  -lgdi32     → 安装界面绘图
  -lmsimg32   → 渐变填充
  -lcomctl32  → 通用控件
  -lcomdlg32  → 文件对话框
  -ladvapi32  → 注册表操作
  -lkernel32  → 文件/内存/进程API

【常见问题】

Q: "undefined reference to `_imp__CreateRoundRectRgn@24'"
A: -lgdi32 没有被链接。检查 Project Options → Parameters → Linker 框
   中是否有 -lgdi32 -lmsimg32。

Q: "undefined reference to `_imp__GradientFill@24'"
A: -lmsimg32 没有被链接。在 Linker 框中添加 -lmsimg32。

Q: "undefined reference to `_imp__CoInitializeEx@8'"
A: -lole32 没有被链接。在 Linker 框中添加 -lole32 -luuid。

Q: 还是报错怎么办？
A: 把 Linker 框中的内容全部替换为上面方法A中的完整内容，
   点击 OK，重新编译。

【编译成功后】

- app.exe 放在项目目录（或其中的子目录）
- installer.exe 放在项目目录
- 双击 installer.exe 运行安装程序
