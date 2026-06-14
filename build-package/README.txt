===============================================================
  抽人软件 - 编译步骤说明
===============================================================

说明：去掉控制台黑窗的唯一办法是让链接器接收 -mwindows 参数。
      以下步骤保证 100% 成功。

===============================================================

【第一步：检查 Dev-C++ 编译器路径】
===============================================================

  1. 打开 Dev-C++
  2. 菜单: Tools -> Compiler Options...  （工具 -> 编译器选项）
  3. 点击第二个标签: Programs  （程序）
  4. 确保四个路径正确：

     gcc:      C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\gcc.exe
     g++:      C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe
     make:     C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\mingw32-make.exe
     windres:  C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe

  5. 点 OK

===============================================================

【第二步：编译 App.exe（主程序悬浮按钮）】
===============================================================

  方法 A（推荐）- 打开现有的 App.dev 项目：

  1. File -> Open Project or File...  （文件 -> 打开项目或文件）
  2. 选择 App.dev 打开
  3. 菜单: Project -> Project Options...  （项目 -> 项目选项）
  4. 点击第二个标签: Parameters  （参数）
  5. 你会看到 4 个输入框，按如下填写：

     ┌───────────────────────────────────────────────────────┐
     │ C++ compiler (C++ 编译器):    -O2                     │
     │                                                         │
     │ Linker (链接器):              -mwindows -luser32     │
     │                             -lgdi32 -lmsimg32       │
     │                             -lshell32 -lcomctl32    │
     │                                                         │
     │ (其他两个框保持原样或留空)                              │
     └───────────────────────────────────────────────────────┘

     重点：-mwindows 必须在 Linker 框里！
          所有 -lxxx 也必须在 Linker 框里！

  6. 点 OK 保存设置
  7. 菜单: Execute -> Compile   （或直接按 Ctrl+F9）
  8. 编译成功后，项目目录中会生成 App.exe

  9. 关闭当前项目: File -> Close Project or File
     （重要：先编译完 App.exe，再编译 Installer）

===============================================================

【第三步：编译 Installer.exe（安装程序）】
===============================================================

  1. File -> Open Project or File...
  2. 选择 Installer.dev 打开
  3. 菜单: Project -> Project Options...
  4. Parameters 标签，Linker 框粘贴：

     -mwindows -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

     C++ compiler 框: -O2

  5. 点 OK
  6. Ctrl+F9 编译
  7. 成功后生成 Installer.exe

===============================================================

【第四步：验证黑窗是否消除】
===============================================================

  1. 关闭 Dev-C++
  2. 在项目目录中找到 App.exe
  3. 双击运行
  4. 不应再看到黑色控制台窗口（只有桌面右下角的悬浮按钮）

  如果仍然有黑窗，检查：
  - Linker 框里确实有 -mwindows
  - 源代码中 #pragma comment(linker, "-mwindows") 已生效
  - 重新 Clean + Compile （Execute -> Clean，再 Ctrl+F9）

===============================================================

【如果编译时出现 undefined reference 链接错误】
===============================================================

  错误形式：
  undefined reference to `_imp__CreateRoundRectRgn@24'
  undefined reference to `_imp__GradientFill@24'
  undefined reference to `IID_IShellLinkW'

  原因：-lxxx 库参数没有传递给链接器

  解决：
  1. Project -> Project Options... -> Parameters
  2. 把所有 -lxxx 参数全部放到 Linker 框里
  3. 不要放在 C++ compiler 框里
  4. 在 C++ compiler 框中，只能有 -O2 或其他编译标志

  对于 App.exe，Linker 框必须有：
  -mwindows -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

  对于 Installer.exe，Linker 框必须有：
  -mwindows -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

===============================================================

【项目文件清单】
===============================================================

  app.cpp            - 主程序源代码
  installer.cpp      - 安装程序源代码
  app.rc             - 主程序资源文件（图标）
  installer.rc       - 安装程序资源文件（图标 + 嵌入 App.exe）
  manifest.xml       - Windows 程序清单（可选）
  app.ico            - 程序图标
  App.dev            - Dev-C++ 主程序项目文件
  Installer.dev      - Dev-C++ 安装程序项目文件
  README.txt         - 本说明文件

===============================================================

【替换为你自己的图标】
===============================================================

  1. 准备好 .ico 文件（建议尺寸 32x32、48x48、64x64）
  2. 将文件命名为 app.ico
  3. 替换项目目录中的 app.ico
  4. 重新编译即可

===============================================================
