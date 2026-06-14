================================================================

  最后一步：去掉运行时的黑窗

  ==================================================================

  问题：双击 App.exe 或 Installer.exe 后弹出黑色控制台窗口
  原因：链接时没有应用 -mwindows 标志，Windows 把它当控制台程序
  解决：重新编译，确保链接器收到 -mwindows 参数

  ==================================================================


  【方法 A（推荐）：通过 Dev-C++ 重新编译】
  ==================================================================

  步骤 1：重新编译 App.exe

  1. 打开 Dev-C++
  2. File -> Open Project or File... -> 选择 App.dev
  3. 菜单: Project -> Project Options...
  4. 点击第二个标签 "Parameters"（参数）
  5. 在 "Linker"（链接器）框中，完整填入下面这一行（作为第一行）

     -mwindows -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

     重要：-mwindows 必须出现在 Linker 框里！
           它告诉 Windows 这是 GUI 程序，不要创建控制台窗口。

  6. 确认 "C compiler" 和 "C++ compiler" 框里只有 -O2（不要有 -lxxx）
  7. 点 OK
  8. 按 Ctrl+F9 重新编译
  9. 编译完成后，关闭项目 (File -> Close Project or File)

  步骤 2：重新编译 Installer.exe

  1. File -> Open Project or File... -> 选择 Installer.dev
  2. 菜单: Project -> Project Options... -> Parameters 标签
  3. 在 "Linker" 框中，完整填入：

     -mwindows -static-libgcc -static-libstdc++ -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

  4. 确认 "C compiler" 和 "C++ compiler" 框里只有 -O2
  5. 点 OK
  6. 按 Ctrl+F9 重新编译

  步骤 3：验证

  1. 关闭 Dev-C++
  2. 到项目目录，双击新生成的 App.exe
  3. 应该不再弹出黑色控制台窗口，只看到右下角悬浮按钮
  4. 双击 Installer.exe，应该也没有黑窗


  【方法 B：如果方法 A 仍有黑窗】
  ==================================================================

  某些 MinGW 版本对 #pragma comment(linker, "-mwindows") 的支持
  不完善。可以用命令行强制指定，100% 可靠：

  1. 打开 CMD (Win+R，输入 cmd，回车)
  2. 进入源码目录: cd /d E:\802
  3. 一行一行执行下面 4 条命令:

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o app_res.o app.rc

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o App.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o inst_res.o installer.rc

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o Installer.exe installer.cpp inst_res.o -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

  每一条命令的第一个参数就是 -mwindows，确保链接器生成为 GUI 子系统。


  【验证黑窗是否已消除】
  ==================================================================

  双击新生成的 exe：
  - 不应该看到黑色控制台窗口
  - 只应该看到右下角的悬浮按钮（App.exe）或安装界面（Installer.exe）

  如果还有黑窗，请：
  1. 确认你运行的是新编译的 exe（不是旧文件）
  2. 在 Dev-C++ 中菜单: Execute -> Clean
     然后再按 Ctrl+F9 重新编译
  3. 或直接用方法 B 的命令行编译
