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

#define RES_ID_APP_EXE     1001   // 主程序二进制资源（RCDATA）
#define RES_ID_APP_ICO     1002   // 图标二进制资源（RCDATA）

#define INSTALLER_CLASS    L"InstallerWindowClass"
#define APP_REG_NAME       L"QuickWebLauncher"
#define APP_EXE_NAME       L"app.exe"
#define APP_ICO_NAME       L"app.ico"
#define DESKTOP_LNK_NAME   L"抽人软件.lnk"
#define STARTUP_LNK_NAME   L"抽人软件.lnk"

static HWND g_hBtn    = nullptr;
static HWND g_hStatus = nullptr;
static HWND g_hWnd    = nullptr;

static LPCWSTR kUrl = L"https://8zs8.github.io/8/";

// ====== 函数声明 ======
LRESULT CALLBACK InstallerWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    DoInstall();
BOOL    ExtractResourceFile(UINT resourceId, LPCWSTR resourceType,
                            LPCWSTR targetPath);
BOOL    RegisterComponents(LPCWSTR installDir, LPCWSTR exePath);
BOOL    CreateRegistryEntries(LPCWSTR installDir, LPCWSTR exePath);
BOOL    CreateDesktopShortcut(LPCWSTR targetPath);
BOOL    CreateStartupShortcut(LPCWSTR targetPath);
BOOL    CreateShortcut(LPCWSTR targetPath, LPCWSTR shortcutFolder,
                       LPCWSTR name);
BOOL    EnsureDirExists(LPCWSTR path);
void    SetStatusText(LPCWSTR text);
void    ShowInstallCompleteUI();

// ====== 入口 ======
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
    wc.hbrBackground = CreateSolidBrush(RGB(15, 23, 42));
    wc.lpszClassName = INSTALLER_CLASS;
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    wc.style         = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"注册窗口类失败", L"错误", MB_ICONERROR | MB_OK);
        return 1;
    }

    int w = 560, h = 380;
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

// ====== 窗口过程 ======
LRESULT CALLBACK InstallerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HFONT s_hTitleFont  = nullptr;
    static HFONT s_hSubFont    = nullptr;
    static HFONT s_hMidFont    = nullptr;
    static HFONT s_hBtnFont    = nullptr;
    static HFONT s_hStatusFont = nullptr;

    switch (message)
    {
        case WM_CREATE:
        {
            HINSTANCE hInst = ((LPCREATESTRUCTW)lParam)->hInstance;

            // 四个文本控件
            CreateWindowW(L"STATIC", L"欢迎使用",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 40, 480, 48,
                          hWnd, (HMENU)ID_STATIC_TITLE, hInst, nullptr);

            CreateWindowW(L"STATIC", L"一键安装，快速访问",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 96, 480, 28,
                          hWnd, (HMENU)ID_STATIC_SUB, hInst, nullptr);

            CreateWindowW(L"STATIC", L"抽人软件",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 150, 480, 36,
                          hWnd, nullptr, hInst, nullptr);

            g_hBtn = CreateWindowW(L"BUTTON", L"🚀  快速安装",
                                   WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                                   170, 220, 220, 56,
                                   hWnd, (HMENU)ID_BTN_INSTALL, hInst, nullptr);

            g_hStatus = CreateWindowW(L"STATIC",
                                      L"点击上方按钮开始安装",
                                      WS_VISIBLE | WS_CHILD | SS_CENTER,
                                      40, 300, 480, 24,
                                      hWnd, (HMENU)ID_STATIC_STATUS, hInst, nullptr);

            // 字体
            s_hTitleFont  = CreateFontW(36, 0, 0, 0, FW_BOLD,     FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
            s_hSubFont    = CreateFontW(18, 0, 0, 0, FW_NORMAL,   FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
            s_hMidFont    = CreateFontW(28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
            s_hBtnFont    = CreateFontW(20, 0, 0, 0, FW_BOLD,     FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
            s_hStatusFont = CreateFontW(16, 0, 0, 0, FW_NORMAL,   FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

            // 按控件 ID 分配字体
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            int unassignedIdx = 0;
            while (hChild)
            {
                LONG_PTR id = GetWindowLongPtrW(hChild, GWLP_ID);
                if (id == ID_STATIC_TITLE)
                    SendMessageW(hChild, WM_SETFONT, (WPARAM)s_hTitleFont, TRUE);
                else if (id == ID_STATIC_SUB)
                    SendMessageW(hChild, WM_SETFONT, (WPARAM)s_hSubFont, TRUE);
                else if (id == ID_STATIC_STATUS)
                    SendMessageW(hChild, WM_SETFONT, (WPARAM)s_hStatusFont, TRUE);
                else if (id == ID_BTN_INSTALL)
                    SendMessageW(hChild, WM_SETFONT, (WPARAM)s_hBtnFont, TRUE);
                else
                {
                    if (unassignedIdx == 0) // "抽人软件" 那个应用名称标签
                        SendMessageW(hChild, WM_SETFONT, (WPARAM)s_hMidFont, TRUE);
                    unassignedIdx++;
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

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lp = (LPDRAWITEMSTRUCT)lParam;
            if (!lp || lp->CtlType != ODT_BUTTON) break;

            RECT rc = lp->rcItem;
            HDC hdc = lp->hDC;

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
                UpdateWindow(g_hStatus);

                if (DoInstall())
                    ShowInstallCompleteUI();
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
            if (s_hTitleFont)  DeleteObject(s_hTitleFont);
            if (s_hSubFont)    DeleteObject(s_hSubFont);
            if (s_hMidFont)    DeleteObject(s_hMidFont);
            if (s_hBtnFont)    DeleteObject(s_hBtnFont);
            if (s_hStatusFont) DeleteObject(s_hStatusFont);
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void SetStatusText(LPCWSTR text)
{
    if (g_hStatus)
    {
        SetWindowTextW(g_hStatus, text);
        UpdateWindow(g_hStatus);
    }
}

BOOL EnsureDirExists(LPCWSTR path)
{
    if (PathFileExistsW(path)) return TRUE;
    WCHAR parent[MAX_PATH];
    StringCchCopyW(parent, ARRAYSIZE(parent), path);
    PathRemoveFileSpecW(parent);
    if (wcslen(parent) == 0) return FALSE;
    if (!PathFileExistsW(parent) && !EnsureDirExists(parent))
        return FALSE;
    return CreateDirectoryW(path, nullptr)
        || GetLastError() == ERROR_ALREADY_EXISTS;
}

// 从 PE 资源中提取二进制文件 → 写到 targetPath
BOOL ExtractResourceFile(UINT resourceId, LPCWSTR resourceType,
                         LPCWSTR targetPath)
{
    HMODULE hModule = GetModuleHandleW(nullptr);
    if (!hModule) return FALSE;

    HRSRC hResInfo = FindResourceW(hModule,
                                   MAKEINTRESOURCEW(resourceId),
                                   resourceType);
    if (!hResInfo) return FALSE;

    HGLOBAL hResData = LoadResource(hModule, hResInfo);
    if (!hResData) return FALSE;

    DWORD  size   = SizeofResource(hModule, hResInfo);
    LPVOID pData  = LockResource(hResData);
    if (!pData || size == 0) return FALSE;

    DeleteFileW(targetPath);

    HANDLE hFile = CreateFileW(targetPath, GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, pData, size, &written, nullptr);
    CloseHandle(hFile);

    if (!ok || written != size)
    {
        DeleteFileW(targetPath);
        return FALSE;
    }

    return TRUE;
}

// ====== 注册组件阶段：写注册表 + 快捷方式 + 自启动 ======
BOOL RegisterComponents(LPCWSTR installDir, LPCWSTR exePath)
{
    SetStatusText(L"正在注册组件…");

    if (!CreateRegistryEntries(installDir, exePath))
        return FALSE;

    SetStatusText(L"正在创建桌面快捷方式…");
    CreateDesktopShortcut(exePath);

    SetStatusText(L"正在设置开机自启…");
    CreateStartupShortcut(exePath);

    return TRUE;
}

// ====== 主安装流程：解压 → 注册 ======
BOOL DoInstall()
{
    // —— 1. 确定安装目录 ——
    WCHAR installDir[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr,
                                    SHGFP_TYPE_CURRENT, installDir)))
    {
        StringCchCopyW(installDir, ARRAYSIZE(installDir), L"C:\\Program Files");
    }
    StringCchCatW(installDir, ARRAYSIZE(installDir), L"\\QuickWebLauncher");

    if (!EnsureDirExists(installDir))
    {
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

    // —— 2. 第一阶段：从安装程序资源中"解压"主程序与图标 ——
    SetStatusText(L"正在解压主程序…");

    WCHAR dstExe[MAX_PATH];
    StringCchCopyW(dstExe, ARRAYSIZE(dstExe), installDir);
    PathAppendW(dstExe, APP_EXE_NAME);

    if (!ExtractResourceFile(RES_ID_APP_EXE, RT_RCDATA, dstExe))
    {
        WCHAR msg[512];
        StringCchPrintfW(msg, ARRAYSIZE(msg),
                         L"解压失败（目标：%s）", dstExe);
        MessageBoxW(g_hWnd, msg, L"安装失败", MB_ICONERROR | MB_OK);
        return FALSE;
    }

    // 可选：如果图标资源存在，则同样解压一份
    WCHAR dstIcon[MAX_PATH];
    StringCchCopyW(dstIcon, ARRAYSIZE(dstIcon), installDir);
    PathAppendW(dstIcon, APP_ICO_NAME);
    ExtractResourceFile(RES_ID_APP_ICO, RT_RCDATA, dstIcon); // 失败不致命

    // —— 3. 第二阶段：注册组件（注册表 + 快捷方式 + 自启动） ——
    if (!RegisterComponents(installDir, dstExe))
    {
        MessageBoxW(g_hWnd, L"注册组件失败", L"安装失败",
                    MB_ICONERROR | MB_OK);
        return FALSE;
    }

    // —— 4. 启动主程序 ——
    SetStatusText(L"正在启动应用…");
    INT_PTR res = (INT_PTR)ShellExecuteW(nullptr, L"open", dstExe,
                                         nullptr, installDir, SW_SHOWNORMAL);
    if (res <= 32)
    {
        WCHAR msg[512];
        StringCchPrintfW(msg, ARRAYSIZE(msg),
                         L"安装完成，但启动应用失败（错误码 %ld）。"
                         L"请双击桌面上的“抽人软件”手动启动。",
                         (LONG)res);
        MessageBoxW(g_hWnd, msg, L"完成", MB_ICONINFORMATION | MB_OK);
    }

    return TRUE;
}

// ====== 注册表：应用注册 + 卸载信息 ======
BOOL CreateRegistryEntries(LPCWSTR installDir, LPCWSTR exePath)
{
    WCHAR regPath[256];
    StringCchPrintfW(regPath, ARRAYSIZE(regPath),
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s",
                     APP_REG_NAME);

    HKEY hRoot = HKEY_LOCAL_MACHINE;
    HKEY hKey  = nullptr;
    if (RegCreateKeyExW(hRoot, regPath, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                        &hKey, nullptr) != ERROR_SUCCESS)
    {
        hRoot = HKEY_CURRENT_USER;
        if (RegCreateKeyExW(hRoot, regPath, 0, nullptr,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                            &hKey, nullptr) != ERROR_SUCCESS)
            return FALSE;
    }

    WCHAR displayIcon[MAX_PATH];
    StringCchPrintfW(displayIcon, ARRAYSIZE(displayIcon), L"%s,0", exePath);

    RegSetValueExW(hKey, L"DisplayName",     0, REG_SZ,
                   (BYTE*)L"抽人软件",
                   (DWORD)(wcslen(L"抽人软件") + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"DisplayIcon",     0, REG_SZ,
                   (BYTE*)displayIcon,
                   (DWORD)(wcslen(displayIcon) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"DisplayVersion",  0, REG_SZ,
                   (BYTE*)L"1.0.0",
                   (DWORD)(wcslen(L"1.0.0") + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ,
                   (BYTE*)installDir,
                   (DWORD)(wcslen(installDir) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"Publisher",       0, REG_SZ,
                   (BYTE*)L"8zs8",
                   (DWORD)(wcslen(L"8zs8") + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"URLInfoAbout",    0, REG_SZ,
                   (BYTE*)kUrl,
                   (DWORD)(wcslen(kUrl) + 1) * sizeof(WCHAR));

    DWORD sizeMB = 1;
    RegSetValueExW(hKey, L"EstimatedSize", 0, REG_DWORD,
                   (BYTE*)&sizeMB, sizeof(DWORD));

    // 卸载命令：最简单、通用的做法 —— 删除目标文件夹内的文件 + 移除注册表项。
    // 这里用一个能工作的简化命令：用 cmd /c rd /s /q 删除目录，实际产品可替换为独立 uninstall.exe。
    WCHAR uninstallCmd[MAX_PATH * 2];
    StringCchPrintfW(uninstallCmd, ARRAYSIZE(uninstallCmd),
                      L"cmd.exe /c \"\"%s\\%s\" --uninstall\"",
                      installDir, APP_EXE_NAME);
    RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ,
                   (BYTE*)uninstallCmd,
                   (DWORD)(wcslen(uninstallCmd) + 1) * sizeof(WCHAR));

    RegCloseKey(hKey);

    // 应用自身信息（HKCU）
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
        RegSetValueExW(hkApp, L"ExePath", 0, REG_SZ,
                       (BYTE*)exePath,
                       (DWORD)(wcslen(exePath) + 1) * sizeof(WCHAR));
        RegCloseKey(hkApp);
    }

    return TRUE;
}

// ====== 快捷方式创建 ======
BOOL CreateShortcut(LPCWSTR targetPath, LPCWSTR shortcutFolder, LPCWSTR name)
{
    WCHAR lnkPath[MAX_PATH];
    StringCchCopyW(lnkPath, ARRAYSIZE(lnkPath), shortcutFolder);
    PathAppendW(lnkPath, name);

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IShellLinkW* psl = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
    if (FAILED(hr)) return FALSE;

    psl->SetPath(targetPath);

    WCHAR workDir[MAX_PATH];
    StringCchCopyW(workDir, ARRAYSIZE(workDir), targetPath);
    PathRemoveFileSpecW(workDir);
    psl->SetWorkingDirectory(workDir);

    psl->SetDescription(L"抽人软件 —— 快速访问");
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
    return SUCCEEDED(hr);
}

BOOL CreateDesktopShortcut(LPCWSTR targetPath)
{
    WCHAR desk[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr,
                                    SHGFP_TYPE_CURRENT, desk)))
        return FALSE;
    return CreateShortcut(targetPath, desk, DESKTOP_LNK_NAME);
}

BOOL CreateStartupShortcut(LPCWSTR targetPath)
{
    WCHAR startup[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr,
                                    SHGFP_TYPE_CURRENT, startup)))
        return FALSE;
    return CreateShortcut(targetPath, startup, STARTUP_LNK_NAME);
}

void ShowInstallCompleteUI()
{
    SetStatusText(L"✅  安装完成！桌面上的 “抽人软件” 已创建。");
    if (g_hBtn)
    {
        SetWindowTextW(g_hBtn, L"完成");
        SetWindowLongPtrW(g_hBtn, GWLP_ID, ID_BTN_CLOSE);
        EnableWindow(g_hBtn, TRUE);
        InvalidateRect(g_hBtn, nullptr, TRUE);
    }
}
