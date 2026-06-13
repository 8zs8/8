# Quick Web Launcher — Windows 托盘 + 置顶按钮

一键安装、在系统右下角显示一个置顶按钮和托盘图标，点击后打开
`https://8zs8.github.io/8/`。

## 文件清单

| 文件 | 作用 |
|---|---|
| `app.cpp` / `app.rc` | 主程序：后台运行（无任务栏窗口），同时显示一个可拖动的置顶悬浮按钮和一个托盘图标 |
| `installer.cpp` / `installer.rc` | 安装程序：美观界面 + 一个 “快速安装” 按钮 |
| `manifest.xml` | 应用程序清单，保证使用通用控件第 6 版（避免按钮发灰） |
| `app.ico` | 你的自定义图标（用户自备，需放在本目录） |
| `build.bat` | MinGW 一键构建脚本 |
| `CMakeLists.txt` | MSVC / CLion / VS Code 的 CMake 工程 |

## 自定义图标

将你自己的 `.ico` 文件重命名为 `app.ico` 放在本目录即可。
该图标会同时作为：

- 安装程序/主程序窗口图标
- 任务栏预览框左上角图标
- 托盘（通知区域）图标
- 资源管理器中 exe 文件图标
- 桌面快捷方式图标
- 任务管理器图标

## 使用 MinGW 构建

1. 安装 **TDM-GCC** 或 **MSYS2 + mingw-w64-x86_64-gcc**，并把 `bin` 加入 `PATH`
2. 将 `app.ico` 放在本目录
3. 双击运行 `build.bat`

构建后会生成：

- `app.exe` —— 主程序
- `installer.exe` —— 安装程序

## 使用 MSVC / Visual Studio 构建

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 使用安装程序

将 `installer.exe`、`app.exe`、`app.ico` 放置在同一文件夹中，
双击 `installer.exe` 即可。安装程序会自动：

1. 把文件复制到 `C:\Program Files\QuickWebLauncher\`（无管理员权限时回退到 `%LocalAppData%\QuickWebLauncher\`）
2. 写入注册表（添加到 “程序和功能” 列表，注册应用信息）
3. 创建桌面快捷方式
4. 创建开机自启快捷方式
5. 启动主程序

卸载可在 “设置 → 应用 → 已安装的应用” 或 “控制面板 → 程序和功能” 中找到 `QuickWebLauncher` 条目。

## 主程序特性

- 完全后台运行，任务栏不出现任何条目
- 屏幕右下角显示一个置顶悬浮按钮（可拖动），点击打开网页
- 系统托盘（通知区域）显示图标，点击左键也可打开网页，右键可退出
- 图标使用资源脚本中的 `1 ICON "app.ico"`，保证所有 UI 都统一
