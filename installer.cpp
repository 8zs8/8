#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <shlobj.h>
#include <stdlib.h>
#include <wchar.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

#define ICON_RESOURCE_ID   1
#define ID_BTN_INSTALL     2001
#define ID_STATIC_TITLE    2002
#define ID_STATIC_SUB      2003
#define ID_STATIC_STATUS   2004
#define ID_BTN_CLOSE       2005

#define INSTALLER_CLASS    L"InstallerWindowClass"
#define APP_REG_NAME       L"QuickWebLauncher"
#define APP_EXE_NAME       L"app.exe"

static HWND g_hBtn     = nullptr;
static HWND g_hStatus  = nullptr;
static HWND g_hWnd     = nullptr;

static LPCWSTR kUrl = L"https://8zs8.github.io/8/";

LRESULT CALLBACK InstallerWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    DoInstall();
BOOL    CreateRegistryEntries(LPCWSTR installDir, LPCWSTR exePath);
BOOL    CreateDesktopShortcut(LPCWSTR targetPath);
BOOL    CreateStartupShortcut(LPCWSTR targetPath);
BOOL    EnsureDirExists(LPCWSTR path);
BOOL    CopyFileEx2(LPCWSTR src, LPCWSTR dst);
void    SetStatusText(LPCWSTR text);
void    ShowInstallCompleteUI();

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE /*hPrevInstance*/,
                   _In_ LPSTR     /*lpCmdLine*/,
                   _In_ int       /*nShowCmd*/)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = InstallerWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(15, 23, 42)); // 深蓝黑
    wc.lpszClassName = INSTALLER_CLASS;
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    wc.style         = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"注册窗口类失败", L"错误", MB_ICONERROR | MB_OK);
        return 1;
    }

    int w = 560;
    int h = 380;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_hWnd = CreateWindowExW(
        WS_EX_WINDOWEDGE | WS_EX_LAYERED,
        INSTALLER_CLASS,
        L"快速安装",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        sx, sy, w, h,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!g_hWnd)
    {
        MessageBoxW(nullptr, L"创建窗口失败", L"错误", MB_ICONERROR | MB_OK);
        return 1;
    }

    // 设置透明圆角效果（简化实现：仅设置半透明）
    SetLayeredWindowAttributes(g_hWnd, 0, (BYTE)(255), LWA_ALPHA);

    SendMessageW(g_hWnd, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));
    SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK InstallerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            HINSTANCE hInst = ((LPCREATESTRUCTW)lParam)->hInstance;

            // 标题
            CreateWindowW(L"STATIC", L"欢迎使用",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 40, 480, 48,
                          hWnd, (HMENU)ID_STATIC_TITLE, hInst, nullptr);

            // 副标题
            CreateWindowW(L"STATIC", L"一键安装，快速访问",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 96, 480, 28,
                          hWnd, (HMENU)ID_STATIC_SUB, hInst, nullptr);

            // 大标题（应用名称）
            CreateWindowW(L"STATIC", L"Quick Web Launcher",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 150, 480, 36,
                          hWnd, nullptr, hInst, nullptr);

            // "快速安装"按钮
            g_hBtn = CreateWindowW(L"BUTTON", L"🚀  快速安装",
                                   WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                   170, 220, 220, 56,
                                   hWnd, (HMENU)ID_BTN_INSTALL, hInst, nullptr);

            // 状态文本
            g_hStatus = CreateWindowW(L"STATIC",
                                      L"点击上方按钮开始安装",
                                      WS_VISIBLE | WS_CHILD | SS_CENTER,
                                      40, 300, 480, 24,
                                      hWnd, (HMENU)ID_STATIC_STATUS, hInst, nullptr);

            // 设置字体（使用系统默认 GUI 字体即可，现代 Windows 会渲染较好）
            HFONT hTitleFont = CreateFontW(36, 0, 0, 0, FW_BOLD,
                                            FALSE, FALSE, FALSE,
                                            DEFAULT_CHARSET,
                                            OUT_DEFAULT_PRECIS,
                                            CLIP_DEFAULT_PRECIS,
                                            DEFAULT_QUALITY,
                                            DEFAULT_PITCH | FF_DONTCARE,
                                            L"Microsoft YaHei UI");

            HFONT hSubFont   = CreateFontW(18, 0, 0, 0, FW_NORMAL,
                                           FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET,
                                           OUT_DEFAULT_PRECIS,
                                           CLIP_DEFAULT_PRECIS,
                                           DEFAULT_QUALITY,
                                           DEFAULT_PITCH | FF_DONTCARE,
                                           L"Microsoft YaHei UI");

            HFONT hMidFont   = CreateFontW(28, 0, 0, 0, FW_SEMIBOLD,
                                           FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET,
                                           OUT_DEFAULT_PRECIS,
                                           CLIP_DEFAULT_PRECIS,
                                           DEFAULT_QUALITY,
                                           DEFAULT_PITCH | FF_DONTCARE,
                                           L"Microsoft YaHei UI");

            HFONT hBtnFont   = CreateFontW(20, 0, 0, 0, FW_BOLD,
                                           FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET,
                                           OUT_DEFAULT_PRECIS,
                                           CLIP_DEFAULT_PRECIS,
                                           DEFAULT_QUALITY,
                                           DEFAULT_PITCH | FF_DONTCARE,
                                           L"Microsoft YaHei UI");

            HFONT hStatusFont = CreateFontW(16, 0, 0, 0, FW_NORMAL,
                                            FALSE, FALSE, FALSE,
                                            DEFAULT_CHARSET,
                                            OUT_DEFAULT_PRECIS,
                                            CLIP_DEFAULT_PRECIS,
                                            DEFAULT_QUALITY,
                                            DEFAULT_PITCH | FF_DONTCARE,
                                            L"Microsoft YaHei UI");

            // 按 Z 序分配字体
            HWND hTitleText  = GetDlgItem(hWnd, ID_STATIC_TITLE);
            HWND hSubText    = GetDlgItem(hWnd, ID_STATIC_SUB);
            HWND hAppName    = GetDlgItem(hWnd, nullptr);   // 不使用

            // 找到第三个 static（应用名称窗口）— 其实上面创建时没有HMENU，用下一个子窗口
            // 简化：遍历所有子窗口
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            int idx = 0;
            while (hChild)
            {
                HMENU id = GetMenu(hChild);
                if (id == (HMENU)ID_STATIC_TITLE)
                    SendMessageW(hChild, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
                else if (id == (HMENU)ID_STATIC_SUB)
                    SendMessageW(hChild, WM_SETFONT, (WPARAM)hSubFont, TRUE);
                else if (id == (HMENU)ID_STATIC_STATUS)
                    SendMessageW(hChild, WM_SETFONT, (WPARAM)hStatusFont, TRUE);
                else if (id == (HMENU)ID_BTN_INSTALL)
                    SendMessageW(hChild, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
                else if (id == 0 || id == nullptr)
                {
                    // 应用名称（唯一未指定 ID 的 static）
                    if (idx == 0)
                        SendMessageW(hChild, WM_SETFONT, (WPARAM)hMidFont, TRUE);
                    idx++;
                }
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }

            return 0;
        }

        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(240, 248, 255));
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        case WM_CTLCOLORBTN:
        {
            HDC hdc = (HDC)wParam;
            HWND hbtn = (HWND)lParam;

            static HBRUSH s_hBtnBrush = nullptr;
            if (!s_hBtnBrush)
                s_hBtnBrush = CreateSolidBrush(RGB(56, 130, 246));

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)s_hBtnBrush;
        }

        case WM_DRAWITEM:
        {
            // 按钮自绘，以实现渐变+圆角外观
            LPDRAWITEMSTRUCT lp = (LPDRAWITEMSTRUCT)lParam;
            if (!lp || lp->CtlType != ODT_BUTTON)
                break;

            RECT rc = lp->rcItem;
            HDC hdc = lp->hDC;

            // 背景渐变
            TRIVERTEX vert[2];
            vert[0].x     = rc.left;
            vert[0].y     = rc.top;
            vert[0].Red   = 0x38 << 8;
            vert[0].Green = 0x6E << 8;
            vert[0].Blue  = 0xF6 << 8;
            vert[0].Alpha = 0xFF << 8;

            vert[1].x     = rc.right;
            vert[1].y     = rc.bottom;

            if (lp->itemState & ODS_SELECTED)
            {
                vert[1].Red   = 0x1D << 8;
                vert[1].Green = 0x4E << 8;
                vert[1].Blue  = 0xDC << 8;
            }
            else if (lp->itemState & ODS_HOTLIGHT)
            {
                vert[1].Red   = 0x5B << 8;
                vert[1].Green = 0x8A << 8;
                vert[1].Blue  = 0xFF << 8;
            }
            else
            {
                vert[1].Red   = 0x25 << 8;
                vert[1].Green = 0x63 << 8;
                vert[1].Blue  = 0xF4 << 8;
            }
            vert[1].Alpha = 0xFF << 8;

            GRADIENT_RECT grect;
            grect.UpperLeft  = 0;
            grect.LowerRight = 1;

            GradientFill(hdc, vert, 2, &grect, 1, GRADIENT_FILL_RECT_V);

            // 文本
            WCHAR text[256];
            GetWindowTextW(lp->hwndItem, text, ARRAYSIZE(text));
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);

            RECT textRc = rc;
            DrawTextW(hdc, text, -1, &textRc,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            return TRUE;
        }

        case WM_COMMAND:
        {
            WORD id = LOWORD(wParam);
            if (id == ID_BTN_INSTALL)
            {
                SetStatusText(L"正在安装，请稍候…");
                EnableWindow(g_hBtn, FALSE);

                if (DoInstall())
                {
                    ShowInstallCompleteUI();
                }
                else
                {
                    SetStatusText(L"安装失败，请以管理员身份运行后重试。");
                    EnableWindow(g_hBtn, TRUE);
                }
            }
            else if (id == ID_BTN_CLOSE)
            {
                PostMessageW(hWnd, WM_CLOSE, 0, 0);
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void SetStatusText(LPCWSTR text)
{
    if (g_hStatus) SetWindowTextW(g_hStatus, text);
}

BOOL EnsureDirExists(LPCWSTR path)
{
    if (PathFileExistsW(path)) return TRUE;
    // 递归创建
    WCHAR parent[MAX_PATH];
    StringCchCopyW(parent, ARRAYSIZE(parent), path);
    PathRemoveFileSpecW(parent);
    if (wcslen(parent) == 0) return FALSE;
    if (!PathFileExistsW(parent) && !EnsureDirExists(parent))
        return FALSE;
    return CreateDirectoryW(path, nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

BOOL CopyFileEx2(LPCWSTR src, LPCWSTR dst)
{
    // 先尝试删除旧文件
    DeleteFileW(dst);
    return CopyFileW(src, dst, FALSE);
}

BOOL CreateRegistryEntries(LPCWSTR installDir, LPCWSTR exePath)
{
    // HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\APP
    // 以及 HKCU\Software\APP  应用注册信息
    WCHAR regPath[256];
    StringCchPrintfW(regPath, ARRAYSIZE(regPath),
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s",
                     APP_REG_NAME);

    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, regPath, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                        &hKey, nullptr) != ERROR_SUCCESS)
    {
        // 若 HKLM 失败（非管理员），退回到 HKCU
        if (RegCreateKeyExW(HKEY_CURRENT_USER, regPath, 0, nullptr,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                            &hKey, nullptr) != ERROR_SUCCESS)
            return FALSE;
    }

    WCHAR displayIcon[MAX_PATH];
    StringCchPrintfW(displayIcon, ARRAYSIZE(displayIcon), L"%s,0", exePath);

    WCHAR uninstallCmd[MAX_PATH * 2];
    StringCchPrintfW(uninstallCmd, ARRAYSIZE(uninstallCmd),
                      L"Rundll32.exe dfshim.dll,ShArpMaintain %s",
                      exePath);

    RegSetValueExW(hKey, L"DisplayName",     0, REG_SZ,
                   (BYTE*)APP_REG_NAME, (DWORD)(wcslen(APP_REG_NAME) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"DisplayIcon",     0, REG_SZ,
                   (BYTE*)displayIcon, (DWORD)(wcslen(displayIcon) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"DisplayVersion",  0, REG_SZ,
                   (BYTE*)L"1.0.0", (DWORD)(wcslen(L"1.0.0") + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ,
                   (BYTE*)installDir, (DWORD)(wcslen(installDir) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"Publisher",       0, REG_SZ,
                   (BYTE*)L"8zs8", (DWORD)(wcslen(L"8zs8") + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"URLInfoAbout",    0, REG_SZ,
                   (BYTE*)kUrl, (DWORD)(wcslen(kUrl) + 1) * sizeof(WCHAR));

    DWORD sizeEstimate = sizeof(DWORD);
    DWORD sizeMB = 1;
    RegSetValueExW(hKey, L"EstimatedSize", 0, REG_DWORD,
                   (BYTE*)&sizeMB, sizeEstimate);

    RegCloseKey(hKey);

    // 同时在 HKCU\Software\APP 注册一个简单条目，便于应用自身定位
    WCHAR appKey[256];
    StringCchPrintfW(appKey, ARRAYSIZE(appKey), L"Software\\%s", APP_REG_NAME);
    HKEY hkApp = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, appKey, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                        &hkApp, nullptr) == ERROR_SUCCESS)
    {
        RegSetValueExW(hkApp, L"InstallDir", 0, REG_SZ,
                       (BYTE*)installDir,
                       (DWORD)(wcslen(installDir) + 1) * sizeof(WCHAR));
        RegSetValueExW(hkApp, L"TargetUrl", 0, REG_SZ,
                       (BYTE*)kUrl,
                       (DWORD)(wcslen(kUrl) + 1) * sizeof(WCHAR));
        RegCloseKey(hkApp);
    }

    return TRUE;
}

BOOL CreateShortcut(LPCWSTR targetPath, LPCWSTR shortcutFolder, LPCWSTR name)
{
    WCHAR lnkPath[MAX_PATH];
    StringCchCopyW(lnkPath, ARRAYSIZE(lnkPath), shortcutFolder);
    PathAppendW(lnkPath, name);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IShellLinkW* psl = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, nullptr,
                          CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
    if (FAILED(hr)) return FALSE;

    psl->SetPath(targetPath);
    psl->SetWorkingDirectory(shortcutFolder);
    psl->SetDescription(L"Quick Web Launcher");
    psl->SetIconLocation(targetPath, 0);

    IPersistFile* ppf = nullptr;
    hr = psl->QueryInterface(IID_PPV_ARGS(&ppf));
    if (FAILED(hr))
    {
        psl->Release();
        return FALSE;
    }

    hr = ppf->Save(lnkPath, TRUE);
    ppf->Release();
    psl->Release();

    CoUninitialize();
    return SUCCEEDED(hr);
}

BOOL CreateDesktopShortcut(LPCWSTR targetPath)
{
    WCHAR desk[MAX_PATH];
    if (!SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr,
                          SHGFP_TYPE_CURRENT, desk))
        return FALSE;
    return CreateShortcut(targetPath, desk, L"QuickWebLauncher.lnk");
}

BOOL CreateStartupShortcut(LPCWSTR targetPath)
{
    WCHAR startup[MAX_PATH];
    if (!SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr,
                          SHGFP_TYPE_CURRENT, startup))
        return FALSE;
    return CreateShortcut(targetPath, startup, L"QuickWebLauncher.lnk");
}

BOOL DoInstall()
{
    // 1) 确定安装目录
    WCHAR installDir[MAX_PATH];
    if (!SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr,
                          SHGFP_TYPE_CURRENT, installDir))
        StringCchCopyW(installDir, ARRAYSIZE(installDir), L"C:\\Program Files");
    StringCchCatW(installDir, ARRAYSIZE(installDir), L"\\QuickWebLauncher");

    if (!EnsureDirExists(installDir))
    {
        // 若无权写入 Program Files，退到 Local AppData
        WCHAR local[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr,
                                       SHGFP_TYPE_CURRENT, local)))
        {
            StringCchCopyW(installDir, ARRAYSIZE(installDir), local);
            StringCchCatW(installDir, ARRAYSIZE(installDir), L"\\QuickWebLauncher");
            if (!EnsureDirExists(installDir)) return FALSE;
        }
        else
            return FALSE;
    }

    SetStatusText(L"正在复制文件…");

    // 2) 读取当前安装程序所在目录（即主程序 app.exe 与 图标的来源）
    WCHAR srcDir[MAX_PATH];
    GetModuleFileNameW(nullptr, srcDir, ARRAYSIZE(srcDir));
    PathRemoveFileSpecW(srcDir);

    WCHAR srcExe[MAX_PATH];
    StringCchCopyW(srcExe, ARRAYSIZE(srcExe), srcDir);
    PathAppendW(srcExe, APP_EXE_NAME);

    WCHAR dstExe[MAX_PATH];
    StringCchCopyW(dstExe, ARRAYSIZE(dstExe), installDir);
    PathAppendW(dstExe, APP_EXE_NAME);

    if (!CopyFileEx2(srcExe, dstExe))
    {
        WCHAR msg[512];
        StringCchPrintfW(msg, ARRAYSIZE(msg),
                         L"复制失败：%s → %s", srcExe, dstExe);
        MessageBoxW(g_hWnd, msg, L"安装失败", MB_ICONERROR | MB_OK);
        return FALSE;
    }

    // 同时复制图标文件（可选）
    WCHAR srcIcon[MAX_PATH], dstIcon[MAX_PATH];
    StringCchCopyW(srcIcon, ARRAYSIZE(srcIcon), srcDir);
    PathAppendW(srcIcon, L"app.ico");
    StringCchCopyW(dstIcon, ARRAYSIZE(dstIcon), installDir);
    PathAppendW(dstIcon, L"app.ico");
    if (PathFileExistsW(srcIcon))
        CopyFileEx2(srcIcon, dstIcon);

    SetStatusText(L"正在写入注册表…");
    if (!CreateRegistryEntries(installDir, dstExe))
    {
        MessageBoxW(g_hWnd, L"写入注册表失败", L"安装失败", MB_ICONERROR | MB_OK);
        return FALSE;
    }

    SetStatusText(L"正在创建桌面快捷方式…");
    CreateDesktopShortcut(dstExe);

    SetStatusText(L"正在设置开机自启…");
    CreateStartupShortcut(dstExe);

    SetStatusText(L"正在启动应用…");

    // 启动主程序（后台运行，显示托盘和置顶图标）
    INT_PTR res = (INT_PTR)ShellExecuteW(nullptr, L"open", dstExe,
                                         nullptr, installDir, SW_SHOWNORMAL);
    if (res <= 32)
    {
        WCHAR msg[512];
        StringCchPrintfW(msg, ARRAYSIZE(msg),
                         L"安装完成，但启动应用失败（错误码 %ld）。"
                         L"请手动双击桌面上的快捷方式启动。", (LONG)res);
        MessageBoxW(g_hWnd, msg, L"完成", MB_ICONINFORMATION | MB_OK);
    }

    return TRUE;
}

void ShowInstallCompleteUI()
{
    // 替换按钮与状态
    SetStatusText(L"✅  安装完成！已创建桌面快捷方式并设置开机自启。");

    if (g_hBtn)
    {
        SetWindowTextW(g_hBtn, L"完成");
        SetWindowLongPtrW(g_hBtn, GWLP_ID, ID_BTN_CLOSE);
        EnableWindow(g_hBtn, TRUE);
        InvalidateRect(g_hBtn, nullptr, TRUE);
    }
}
