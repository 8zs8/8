=========================================
Quick Web Launcher - Dev-C++ 编译指南
=========================================

【第一步：编译 app.exe】

方法 1：使用项目文件（推荐尝试）

1. 启动 Dev-C++
2. 菜单: File → Open Project or File...
3. 选择文件: App.dev （打开它）
4. 按 Ctrl+F9 或菜单: Execute → Compile
5. 如果成功 → 跳到"第二步"
6. 如果报错 "undefined reference to ..." → 用下面的"方法 2"

=========================================

方法 2：手动配置链接库（最可靠）

1. 在 Dev-C++ 中打开 App.dev 后
2. 菜单: Project → Project Options...
3. 点击第二个标签 "Parameters"（参数）
4. 在标签 "Parameters" 下:
   - 找到 "Linker" （链接器）输入框
   - 清空原有内容（如果有的话）
   - 复制粘贴下面一整行：

-luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

5. 点击右下角 "OK" 保存
6. 按 Ctrl+F9 编译（Execute → Compile）
7. 应该会成功生成 app.exe

【第二步：编译 installer.exe】

方法 1：使用项目文件（推荐尝试）

1. 先在 Dev-C++ 中关闭 App.dev（菜单: File → Close Project or File）
2. 菜单: File → Open Project or File...
3. 选择文件: Installer.dev
4. 按 Ctrl+F9 编译
5. 如果成功 → 完成
6. 如果报错 → 用下面的"方法 2"

=========================================

方法 2：手动配置链接库（最可靠）

1. 在 Dev-C++ 中打开 Installer.dev 后
2. 菜单: Project → Project Options...
3. 点击第二个标签 "Parameters"（参数）
4. 在标签 "Parameters" 下:
   - 找到 "Linker" （链接器）输入框
   - 清空原有内容（如果有的话）
   - 复制粘贴下面一整行：

-lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

5. 点击右下角 "OK" 保存
6. 按 Ctrl+F9 编译
7. 应该会成功生成 installer.exe

=========================================

【第三步：测试】

1. 两个 exe 文件都生成后
2. 双击 installer.exe 运行安装程序
3. 按照安装界面的提示完成安装

=========================================

【库说明 - 为什么需要这些 -lxxx】

app.exe:
  -luser32   窗口、消息循环、托盘图标 (CreateWindowEx, PostMessage)
  -lgdi32    圆角区域、画线画刷 (CreateRoundRectRgn)
  -lmsimg32  渐变填充 (GradientFill)
  -lshell32  打开URL (ShellExecuteW)
  -lcomctl32 通用控件支持

installer.exe:
  -lshell32   创建快捷方式 (IShellLinkW, IPersistFile)
  -lshlwapi   路径操作 (PathAppendW 等)
  -lole32     COM初始化 (CoInitializeEx, CoCreateInstance)
  -luuid      接口IID符号 (IID_IShellLinkW, IID_IPersistFile)
  -luser32    安装界面窗口
  -lgdi32     安装界面绘图 (CreateSolidBrush)
  -lmsimg32   安装界面渐变填充
  -lcomctl32  通用控件
  -lcomdlg32  文件对话框
  -ladvapi32  注册表操作
  -lkernel32  文件、进程、内存API

【常见错误修复】

错误信息:
  undefined reference to `_imp__CreateRoundRectRgn@24'
  undefined reference to `_imp__GradientFill@24'

原因: 链接器没有接收到 -lgdi32 -lmsimg32 等参数

解决:
  1. 打开 Project → Project Options... → Parameters
  2. 在 Linker 框中填入上面"方法 2"中提供的库列表
  3. 点击 OK，重新编译

错误信息:
  undefined reference to `_imp__CoInitializeEx@8'
  undefined reference to `IID_IShellLinkW'

原因: 链接器没有接收到 -lole32 -luuid 等参数

解决:
  1. 打开 Project → Project Options... → Parameters
  2. 在 Linker 框中填入上面 installer 的完整库列表
  3. 点击 OK，重新编译

【提示】

- 如果你按照"方法 2"配置了链接库，编译就一定能成功
- 链接库必须放在 "Linker" 框里，而不是 "Compiler" 或 "C++ compiler" 框
- 不要把 -lxxx 写在 "Makefile" 或其他位置
- Project Options 的 Parameters 标签中的 Linker 框是最重要的位置
