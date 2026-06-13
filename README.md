# Quick Web Launcher —— 抽人软件

后台常驻 + 快速访问

点击右下角置顶悬浮按钮 或 托盘图标 → 打开 `https://8zs8.github.io/8/`。

## 文件清单

| 文件 | 作用 |
|---|---|
| `app.cpp` / `app.rc` | 主程序：后台运行。界面=悬浮置顶按钮 + 系统托盘图标 |
| `installer.cpp` / `installer.rc` | 安装程序：一键安装，安装流程=解压主程序→注册组件 |
| `manifest.xml` | 应用程序清单，保证使用通用控件第 6 版 |
| `app.ico` | 你的自定义图标（自备，需放在本目录，资源 ID=1） |
| `build.bat` | MinGW 一键构建脚本 |
| `CMakeLists.txt` | MSVC / CLion / VS Code 的 CMake 工程 |

## 桌面快捷方式

- 安装后在桌面创建 `抽人软件.lnk`，并在启动文件夹创建同名自启快捷方式。
- 安装完成后自动启动主程序。

## 安装/卸载

- 安装目录默认 `C:\Program Files\QuickWebLauncher\`（无管理员权限时自动退到 `%LocalAppData%\QuickWebLauncher\`）。
- 注册表：写入 `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\QuickWebLauncher`，可在"设置→应用"或"程序和功能"里看到「抽人软件」条目并卸载。
- 应用自身信息写入 `HKCU\Software\QuickWebLauncher`。

## 构建与使用

### 方式一：MinGW（推荐）

1. 安装 MSYS2 并安装 `mingw-w64-x86_64-gcc`，或安装 TDM-GCC。
2. 将 `bin` 加入 `PATH`。
3. 将自定义图标命名为 `app.ico` 放入本目录。
4. 双击 `build.bat`。
5. 构建结果：
   - `app.exe`（主程序，同时被以二进制资源形式嵌入 installer.exe）
   - `installer.exe`（**只需分发这一个文件给用户**）
6. 用户双击 `installer.exe` 即可一键安装。

### 方式二：MSVC / Visual Studio

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

> ⚠️ 注意：用 CMake 时要**先单独构建 Release 版 app.exe 到项目根目录**，再构建 installer，因为 installer.rc 里的 `1001 RCDATA "app.ico"` 等依赖根目录的 app.exe。推荐配合 Post-build event / 或手动两次构建。

## 安装程序技术要点

1. `ExtractResourceFile(UINT, LPCWSTR, LPCWSTR)`
   - `FindResourceW → LoadResource → LockResource → WriteFile`
   - 从 PE 资源中解压 app.exe / app.ico 写到目标目录。
2. `RegisterComponents(installDir, exePath)`
   - 写注册表、创建桌面快捷方式「抽人软件.lnk」、创建启动项快捷方式。
3. `CreateShortcut` 基于 `IShellLinkW + IPersistFile` 实现，图标指向 app.exe,0，保证桌面图标与应用图标一致。
