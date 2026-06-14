================================================================

  编译指南（Dev-C++ 图形界面操作）

  重要提示：
  1. 编译器路径已经正确（C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe）
  2. 现在的问题：链接库参数没有正确传递给链接器
  3. 请严格按下面的步骤操作，不要省略任何一步

  ==================================================================


  【第一步】编译 App.exe
  ==================================================================

  1. 打开 Dev-C++

  2. 菜单: File -> Open Project or File...
           (文件 -> 打开项目或文件)

  3. 找到并选择文件: App.dev
     点"打开"

  4. 现在最重要的一步: 配置链接库
     菜单: Project -> Project Options...
           (项目 -> 项目选项)

  5. 在弹出的窗口中，点击标签: Parameters  (参数，第二个标签)

  6. 你会看到 4 个输入框。请按下面要求，一个一个检查：

     ┌─────────────────────────────────────────────────────┐
     │  编译器 (C compiler)                                │
     │  内容: -O2                                          │
     │  (只有 -O2，不能有 -lxxx)                           │
     ├─────────────────────────────────────────────────────┤
     │  C++ 编译器 (C++ compiler)                          │
     │  内容: -O2                                          │
     │  (只有 -O2，不能有 -lxxx)                           │
     ├─────────────────────────────────────────────────────┤
     │  链接器 (Linker)                                    │
     │  内容: -mwindows -static-libgcc -static-libstdc++  │
     │        -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
     │  (这个是唯一放 -lxxx 的地方)                       │
     ├─────────────────────────────────────────────────────┤
     │  其他选项 (Other Options - optional)                │
     │  内容: (空) 或保持默认                              │
     └─────────────────────────────────────────────────────┘

     注意：
     - 只有 "Linker" 框可以放 -lxxx
     - "C compiler" 和 "C++ compiler" 框中绝对不能有 -lxxx
     - 如果 "C compiler" 或 "C++ compiler" 框中有 -lxxx，
       请删除它们

  7. 确认 4 个框的内容都正确后，点右下角 OK 保存

  8. 现在编译: 按 Ctrl+F9
     或菜单: Execute -> Compile

  9. 如果看到 "Compilation successful" (编译成功)，恭喜！
     App.exe 已经生成在项目目录中

  10. 如果还是报错 "undefined reference to _imp__..."
      → 重复步骤 4-7，特别检查 Linker 框的内容
      → 或跳到本指南最后的【终极方案】


  【第二步】编译 Installer.exe
  ==================================================================

  1. 菜单: File -> Close Project or File
            (文件 -> 关闭项目或文件)

  2. 菜单: File -> Open Project or File...

  3. 找到并选择文件: Installer.dev

  4. 菜单: Project -> Project Options...

  5. 点击标签: Parameters

  6. 4 个输入框按以下要求填写：

     ┌─────────────────────────────────────────────────────┐
     │  编译器 (C compiler)          │  -O2               │
     │  C++ 编译器 (C++ compiler)    │  -O2               │
     │  链接器 (Linker)              │  -mwindows -static-libgcc -static-libstdc++ -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32
     │  其他选项                     │  (空)              │
     └─────────────────────────────────────────────────────┘

  7. 点 OK 保存

  8. 按 Ctrl+F9 编译

  9. 如果成功，Installer.exe 已经生成


  【第三步】测试
  ==================================================================

  1. 在项目目录中找到: App.exe 和 Installer.exe
  2. 双击 Installer.exe 运行安装程序


  ==================================================================
  【终极方案】如果以上步骤仍无法编译
  ==================================================================

  Dev-C++ 的项目管理对某些版本有兼容性问题。可以直接用
  命令行调用编译器。这是 100% 可靠的方法。

  1. 打开 CMD (Win+R，输入 cmd，回车)

  2. 输入:
     cd /d E:\802
     (或你保存源码的其他目录)

  3. 复制下面 4 条命令，一条一条粘贴到 CMD 中，回车执行:

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o app_res.o app.rc

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o App.exe app.cpp app_res.o -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\windres.exe" -o inst_res.o installer.rc

     "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe" -mwindows -O2 -static-libgcc -static-libstdc++ -o Installer.exe installer.cpp inst_res.o -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

  4. 如果每条命令都没有错误，目录中就会有 App.exe 和 Installer.exe


  ==================================================================
  【常见错误对照表】
  ==================================================================

  错误信息：
    g++.exe: No such file or directory
  原因：
    编译器路径没配置
  解决：
    菜单 Tools -> Compiler Options -> Programs 标签
    把 g++ 的路径设置为:
    C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe

  ──────────────────────────────────────────────────────────────

  错误信息：
    linker input file unused because linking not done
  原因：
    -lxxx 参数写到了 Compiler 框里（编译时不需要 -lxxx）
  解决：
    打开 Project Options -> Parameters
    检查 "C compiler" 和 "C++ compiler" 框，删除里面的 -lxxx
    -lxxx 只能放在 "Linker" 框里

  ──────────────────────────────────────────────────────────────

  错误信息：
    undefined reference to `_imp__CreateRoundRectRgn@24'
    undefined reference to `_imp__GradientFill@24'
  原因：
    链接器没有收到 -lgdi32 -lmsimg32
  解决：
    Project Options -> Parameters -> Linker 框
    填入: -mwindows -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

  ──────────────────────────────────────────────────────────────

  错误信息：
    undefined reference to `_imp__CoInitializeEx@8'
    undefined reference to `IID_IShellLinkW'
  原因：
    Installer 的 Linker 框里缺少 -lole32 -luuid
  解决：
    Project Options -> Parameters -> Linker 框
    填入 Installer 的完整库列表（见【第二步】第6步）
