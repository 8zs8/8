# Quick Web Launcher · 抽人软件

后台常驻 + 快速访问

点击右下角置顶悬浮按钮 或 托盘图标 → 打开 `https://8zs8.github.io/8/`。

---

## 一、准备

1. 把你想要用的图标命名为 **app.ico** 放在项目根目录。
2. 确保你当前使用的 C++ IDE（Dev-C++ / Code::Blocks 等）已经自带 GCC + windres（一般安装后就有）。

## 二、编译（任选一种方式）

### 方式 A · 推荐：双击 compile.bat

在文件管理器中双击 **compile.bat**，脚本会自动按正确顺序完成：

```
[1/4] 编译主程序资源 ...
[2/4] 编译并链接主程序 app.exe ...
[3/4] 编译安装程序资源（此时会把 app.exe 作为二进制资源嵌入） ...
[4/4] 编译并链接安装程序 installer.exe ...
```

> 若出现「g++ 不是内部命令」：打开 Dev-C++ → 工具 → 编译选项 → 目录 → 复制 Bin 目录的完整路径，
> 然后在当前目录打开 cmd，执行：
>
> ```
> set PATH=你复制的bin目录路径;%PATH%
> compile.bat
> ```

### 方式 B · Dev-C++ GUI 方式

```
1. 打开 Dev-C++  →  文件 → 打开项目或文件
2. 选择本目录下的  App.dev  →  按 F9（运行 → 编译）
3. 编译成功后，再打开  Installer.dev  →  按 F9
4. 完成后同目录会出现  app.exe  与  installer.exe
```

⚠️ **顺序很重要**：Installer.dev 需要 app.exe 已存在，因为它会把 app.exe 作为资源嵌入安装程序。

### 方式 C · make / mingw32-make

在 cmd 里进入项目目录：

```
set PATH=C:\Dev-Cpp\MinGW64\bin;%PATH%
mingw32-make
```

---

## 三、产出文件

| 文件 | 作用 | 是否分发给用户 |
|---|---|---|
| **app.exe** | 主程序（托盘 + 置顶悬浮按钮 → 打开目标网页） | 否，已作为资源嵌入 installer.exe |
| **installer.exe** | 一键安装程序（解压 → 注册 → 启动） | ✅ 是，只需要分发这一个文件 |

---

## 四、安装时会做什么

用户双击 installer.exe 后：

```
1. 确定安装目录（默认 C:\Program Files\QuickWebLauncher\，
   无管理员权限时自动退到 %LOCALAPPDATA%\QuickWebLauncher\）
2. 从安装程序自身的 PE 资源中解压 app.exe / app.ico 到安装目录
3. 写入注册表 —— 在"程序和功能"中添加"抽人软件"条目，方便用户卸载
4. 创建桌面快捷方式  抽人软件.lnk
5. 创建启动项快捷方式   抽人软件.lnk  →  开机自启
6. 启动 app.exe
```

启动后屏幕右下角会出现一个置顶悬浮按钮，系统托盘区会出现图标——点击任一都会打开目标网页。

---

## 五、目录结构

```
project/
├── app.cpp              主程序源代码
├── app.rc               主程序资源脚本（图标、版本、清单）
├── installer.cpp        安装程序源代码
├── installer.rc         安装程序资源脚本（把 app.exe 作为二进制嵌入）
├── manifest.xml         Windows 应用清单（Common Controls v6 + DPI 感知）
├── compile.bat          通用一键编译脚本（推荐）
├── App.dev              Dev-C++ 工程 · 主程序
├── Installer.dev        Dev-C++ 工程 · 安装程序
├── Makefile             make / mingw32-make 工程
├── build-guide.html     详细网页版构建指南
└── app.ico              ← 你的自定义图标，必须存在
```

---

## 六、常见问题

**Q: 编译报错「windres: can't open `app.ico'」**
A: 你还没放图标。把你的图标重命名为 app.ico，放在项目根目录。

**Q: 编译报错「'g++' 不是内部或外部命令」**
A: 你的编译器 bin 目录没在 PATH 里。参见上方「方式 A」的临时添加命令。

**Q: 编译成功但 exe 运行后没有悬浮按钮**
A: 检查是否有其他窗口阻挡——悬浮按钮会始终置顶。也可以尝试双击 app.exe 直接运行来验证主程序逻辑。

**Q: 安装后桌面快捷方式的图标是默认方块**
A: 快捷方式图标指向 app.exe,0。请确认 app.ico 是有效的 .ico 格式，并确认 app.exe 确实被成功编译。

---

## 七、主程序功能一览

- [x] 屏幕右下角置顶悬浮按钮（WS_EX_TOPMOST + WS_EX_TOOLWINDOW，不占用任务栏）
- [x] 系统托盘图标（右键 = 关闭 / 打开目标页面）
- [x] 点击悬浮按钮或托盘图标 → 打开目标网页
- [x] 渐变蓝 + 圆角外观
- [x] 可拖动悬浮按钮
- [x] 完全后台运行，不出现在任务栏
- [x] 安装后开机自启
