================================================================

  抽人软件 - 编译指南

  【推荐方法：新建项目】      成功率 99%
  【备用方法：命令行】        100% 可靠

==================================================================


【先检查编译器配置 - 必须做！】
==================================================================

  打开 Dev-C++
  菜单:  Tools -> Compiler Options...   (工具 -> 编译器选项)

  看到弹出窗口，点击第二个标签: "Programs"  (程序)

  你会看到 4 个输入框，必须填成以下路径：

    gcc:      C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\gcc.exe
    g++:      C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe
    make:     C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\mingw32-make.exe
    windres:  C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe

  如果路径不对，手动改正，然后点 OK。

  验证: File -> New -> Source File  新建一个文件，写一行:
         int main() { return 0; }
         按 Ctrl+F9 编译
         如果成功，说明编译器路径正确。


【方法一：通过 .dev 文件打开项目】
==================================================================

  ============
  编译 App.exe
  ============

  1. 菜单:  File -> Open Project or File...
  2. 选择文件:  App.dev
  3. 菜单:  Project -> Project Options...
  4. 点击第二个标签:  Parameters (参数)
  5. 现在你会看到 4 个输入框。**逐个检查并修正**：

     ┌────────────────────────────────────────────────────────┐
     │ 编译器 (C compiler)            -O2                    │
     │                                                ↑只有这 │
     │ C++ 编译器 (C++ compiler)      -O2            个，别加 │
     │                                                别的     │
     ├────────────────────────────────────────────────────────┤
     │ 链接器 (Linker)                 ↓这一整行必须完整粘贴↓ │
     │ -mwindows -static-libgcc -static-libstdc++ -luser32   │
     │  -lgdi32 -lmsimg32 -lshell32 -lcomctl32               │
     ├────────────────────────────────────────────────────────┤
     │ 其他选项 (Other options)       （留空）               │
     └────────────────────────────────────────────────────────┘

     重要！重要！重要！
     "编译器"和"C++编译器"框中只能有 -O2
     "-lxxx" 和 "-mwindows" 只出现在"链接器"框中
     如果任何其他框中有 -lxxx，请删除！

  6. 点 OK
  7. 菜单:  Execute -> Compile   （或按 Ctrl+F9）
  8. 成功后，项目目录中会生成 App.exe

  ============
  编译 Installer.exe
  ============

  1. 先完成上面的 App.exe 编译（Installer 需要嵌入 app.exe）
  2. 菜单:  File -> Close Project or File （关闭当前项目）
  3. 菜单:  File -> Open Project or File...
  4. 选择文件:  Installer.dev
  5. 菜单:  Project -> Project Options...
  6. Parameters 标签，同样检查 4 个框：

     编译器 (C compiler):       -O2
     C++ 编译器 (C++ compiler): -O2
     链接器 (Linker):           -mwindows -static-libgcc -static-libstdc++ -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32
     其他选项:                  （空）

  7. 点 OK
  8. 按 Ctrl+F9 编译
  9. 成功后生成 Installer.exe


【方法二：手动新建项目（最可靠，推荐！）】
==================================================================

  ============
  编译 App.exe
  ============

  1. 菜单:  File -> New -> Project   文件 -> 新建 -> 项目
  2. 在弹出的窗口中:
     - 选择:  Windows Application   （注意：不是 Console Application！）
     - 名称:  App
     - 点 OK
  3. 会弹出一个编辑器，里面有默认的代码。**关闭这个窗口，不要保存**
  4. 菜单:  Project -> Add to Project   项目 -> 添加到项目
  5. 选择文件:  app.cpp   （从解压的源码目录中选）
  6. 再做一次: Project -> Add to Project
  7. 选择文件:  app.rc
  8. 菜单:  Project -> Project Options...   项目 -> 项目选项
  9. Parameters 标签:
     编译器框:        -O2
     C++编译器框:     -O2
     链接器框:        -mwindows -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
     其他选项框:      （空）
  10. 点 OK
  11. 按 Ctrl+F9 编译

  ============
  编译 Installer.exe
  ============

  1. 菜单:  File -> Close Project or File
  2. File -> New -> Project
  3. Windows Application，名称填 Installer
  4. 关闭默认代码窗口（不保存）
  5. Project -> Add to Project，添加: installer.cpp
  6. Project -> Add to Project，添加: installer.rc
  7. Project -> Project Options... -> Parameters
  8. 编译器框: -O2
     C++编译器框: -O2
     链接器框: -mwindows -static-libgcc -static-libstdc++ -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32
  9. 点 OK
  10. 按 Ctrl+F9 编译


【方法三：命令行（最后手段，100%可靠）】
==================================================================

  1. 打开 CMD:  Win+R 键，输入 cmd，回车
  2. 进入源码目录:
     cd /d E:\802     （或你保存源码的实际目录）

  3. 编译 App.exe（复制这一整行，粘贴回车）：

"C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o app_res.o app.rc && "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o App.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

  4. 编译 Installer.exe（复制这一整行，粘贴回车）：

"C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o inst_res.o installer.rc && "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o Installer.exe installer.cpp inst_res.o -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32


【常见错误对照表】
==================================================================

  错误: linker input file unused because linking not done
  原因: -lxxx 被放到了编译器框而不是链接器框
  解决: 打开 Project Options -> Parameters
        删除"编译器"和"C++编译器"框中所有 -lxxx
        所有 -lxxx 只保留在"链接器"框中

  错误: undefined reference to `__imp_CreateRoundRectRgn@24'
  错误: undefined reference to `__imp_GradientFill@24'
  原因: -lgdi32 或 -lmsimg32 没有传递给链接器
  解决: 打开 Project Options -> Parameters -> 链接器框
        确保包含: -lgdi32 -lmsimg32

  错误: undefined reference to `IID_IShellLinkW'
  错误: undefined reference to `_imp_CoInitializeEx@8'
  原因: -lole32 -luuid 没有传递给链接器（installer.exe）
  解决: 打开 Installer 项目的 Project Options -> Parameters -> 链接器
        确保包含: -lole32 -luuid

  错误: g++.exe: No such file or directory
  原因: Dev-C++ 的编译器路径配置错误
  解决: 按照本指南开头的"先检查编译器配置"步骤改正路径

  错误: skipping incompatible .../lib32/libmingw32.a
  原因: Dev-C++ 在搜索 32 位库，但编译器是 64 位的
  解决: 菜单 Tools -> Compiler Options
        在"Settings"或"General"标签中，取消选择 "Use 32-bit libraries"
        或确保编译器配置为 "TDM-GCC 4.9.2 64-bit Release" 类似的 64 位配置

  运行时出现黑色控制台窗口:
  原因: -mwindows 没有传递给链接器
  解决: 打开 Project Options -> Parameters -> 链接器框
        确保第一行就是 -mwindows
        同时 app.cpp 源文件中已有 #pragma comment(linker, "-mwindows")


【测试编译结果】
==================================================================

  编译成功后，目录中会有:
    App.exe        - 主程序（悬浮按钮）
    Installer.exe  - 安装程序

  双击 App.exe 测试:
  - 不应看到黑色控制台窗口（如果有黑窗，说明 -mwindows 没生效）
  - 屏幕右下角应该出现一个小的悬浮按钮
  - 单击悬浮按钮: 打开浏览器访问 https://8zs8.github.io/8/
  - 长按悬浮按钮（0.5秒以上）: 弹出右键菜单
  - 拖动悬浮按钮: 可以移动位置（不会打开网页）
  - 托盘区应该有一个图标，右键托盘图标也有菜单
