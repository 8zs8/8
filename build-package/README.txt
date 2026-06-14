================================================================
  Quick Web Launcher - Dev-C++ 操作手册
  ==================================================================

  【重要】Dev-C++ 5.x 中，编译器路径和库链接参数必须通过
       以下两个地方设置，任何其他地方设置都可能无效：

       1. Tools -> Compiler Options -> Programs    （编译器路径）
       2. Project -> Project Options -> Parameters （库链接参数）

  ==================================================================


  ==================================================================
  第一部分：配置 Dev-C++ 的编译器路径（只需做一次）
  ==================================================================

  问题根源：Dev-C++ 找不到 g++.exe，因为它的编译器路径配置
           指向了错误位置（E:\802\g++.exe）。必须告诉它正确
           的 MinGW 路径。

  操作步骤：

  1. 打开 Dev-C++

  2. 顶部菜单:  Tools -> Compiler Options...
     (工具 -> 编译器选项)

  3. 在弹出的窗口中，点击标签 "Programs" （第二个标签）

  4. 你会看到几个输入框，把它们全部改为以下路径（注意是你
     自己电脑上的真实路径）：

       gcc     = C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\gcc.exe
       g++     = C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe
       make    = C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\mingw32-make.exe
       windres = C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe

     （如果 Dev-C++ 版本较老，上述字段名可能是 C Compiler,
      C++ Compiler, Make, Resource Compiler）

  5. 点击 OK 保存。

  验证：关闭所有项目，菜单 File -> New -> Source File，写一行
        #include <stdio.h>
        int main(){ printf("OK\n"); return 0; }
        按 F9 编译运行。如果能看到 OK 窗口，编译器路径就对了。


  ==================================================================
  第二部分：编译 app.exe
  ==================================================================

  方法 A：使用我提供的 App.dev 项目文件

  1. 菜单: File -> Open Project or File...
  2. 选择文件: App.dev （从解压出来的文件夹里）
  3. 菜单: Project -> Project Options...
  4. 点击标签 "Parameters" （参数，第二个标签）
  5. 在最下面的 "Linker" 框中，完整粘贴下面一整行：

     -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

     （如果 Linker 框里已经有内容，先清空再粘贴。不要把
      这些参数写到 Compiler 或 C++ compiler 框中！）

  6. 点击 OK 保存
  7. 按 Ctrl+F9 编译（或菜单 Execute -> Compile）
  8. 应该会看到 "Compilation successful"，生成 App.exe


  方法 B：从零创建新项目（最可靠，推荐）

  1. 菜单: File -> New -> Project
  2. 在弹出的窗口中，选择 "Windows Application"（NOT Console）
  3. 在 Name 框中填 "App"，选择保存目录（你的源码目录）
  4. 点 OK
  5. 会看到一个默认的 main.c 或 main.cpp 文件，把它关闭（不要保存）
  6. 菜单: Project -> Add to Project
  7. 添加文件: app.cpp 和 app.rc
  8. 菜单: Project -> Project Options... -> Parameters 标签
  9. 在 "Linker" 框中粘贴:
     -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
  10. 点 OK
  11. 按 Ctrl+F9 编译
  12. 生成 App.exe（或 App.exe，取决于项目名）


  方法 C：使用自定义 Makefile（最稳定，如果以上都不行）

  1. 先按"方法 A"或"方法 B"创建/打开项目
  2. 菜单: Project -> Project Options...
  3. 找是否有标签叫 "Makefile"（在某些 Dev-C++ 版本中是最后一个）
  4. 如果有，勾选 "Use custom Makefile" 或类似选项
  5. 在输入框中填: Makefile.win
  6. 点 OK
  7. 按 Ctrl+F9 编译
  8. Dev-C++ 会直接调用 Makefile.win 中的命令，绕过内部生成器


  ==================================================================
  第三部分：编译 installer.exe
  ==================================================================

  步骤与上面完全相同，仅库列表不同：

  在 Project -> Project Options... -> Parameters -> Linker 框中粘贴:

  -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32


  ==================================================================
  第四部分：如果仍然报错 "undefined reference to _imp__..."
  ==================================================================

  原因: 链接器没有收到 -lxxx 参数（Dev-C++ 版本差异导致）

  终极解决办法（任何版本都有效）：

  1. 打开 CMD（Win+R，输入 cmd，回车）
  2. 用 cd 命令进入源码目录，例如:
     cd /d E:\802

  3. 直接运行 g++ 编译器，用完整路径（一次性粘贴 3 条命令）:

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o app_res.o app.rc
     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o app.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o inst_res.o installer.rc
     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o installer.exe installer.cpp inst_res.o -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

  4. 如果成功，目录中会有 app.exe 和 installer.exe

  说明：这个方法是 100% 可靠的，因为它直接调用编译器，完全绕
        过 Dev-C++ 的项目管理和 Makefile 生成机制。之前的所有
        问题都是 Dev-C++ 的项目文件解析与 Makefile 生成有缺陷。


  ==================================================================
  第五部分：常见错误对照表
  ==================================================================

  错误信息                                  | 原因                    | 解决
  -------------------------------------------+-------------------------+---------
  g++.exe: No such file or directory         | 编译器路径错            | 看第一部分
  undefined reference to _imp__CreateRoundRgn| -lgdi32 没传给链接器    | 看第二部分第5步
  undefined reference to _imp__GradientFill  | -lmsimg32 没传给链接器  | 看第二部分第5步
  undefined reference to _imp__CoInitializeEx| -lole32 没传给链接器    | 看第三部分
  undefined reference to IID_IShellLinkW     | -luuid 没传给链接器     | 看第三部分
  recipe for target 'xxx.o' failed           | 通常因为编译命令路径错  | 用第四部分

  记住：所有 "undefined reference to _imp__XXX" 都是【库没传给
  链接器】的问题，不是代码错误。只需在 Project Options 的
  Parameters -> Linker 框里填入正确的 -lxxx 参数即可。
