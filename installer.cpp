// ====================================================================
//  抽人软件 - 安装程序
//  功能：解压主程序、注册表注册、桌面快捷方式、开机自启
//  编译：1. 先编译 App.dev 得到 app.exe
//        2. 打开 Installer.dev，按 Ctrl+F9
// ====================================================================

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commctrl.h>

// --- 链接器指令：确保无控制台窗口 ---
#pragma comment(linker, "-mwindows")

// --- 依赖库 ---
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "kernel32.lib")

// --- 前向声明 ---
LRESULT CALLBACK InstallWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL DoInstallStep(HWND hwnd);
static void PaintInstaller(HWND hwnd, HDC hdc);
static void PathAppend(wchar_t* out, size_t outSize,
                       const wchar_t* base, const wchar_t* extra);
static BOOL CreateShortcutAt(const wchar_t* targetPath,
                           const wchar_t* workingDir,
                           const wchar_t* lnkPath);
static BOOL ExtractResourceFile(UINT resourceId, const wchar_t* targetPath);

#define INSTALLER_WIN_CLASS L"QRInstallerClass"
#define INSTALL_BUTTON_ID   1001

// --- 程序名称 ---
static const wchar_t g_appName[]        = L"抽人软件";
static const wchar_t g_regKeyName[]    = L"Software\\ChouRen";
static const wchar_t g_shortcutName[]    = L"抽人软件.lnk";
static const wchar_t g_targetUrl[]      = L"https://8zs8.github.io/8/";

// ====================================================================
// WinMain
// ====================================================================
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = InstallWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0x14, 0x1A, 0x30));
    wc.lpszClassName = INSTALLER_WIN_CLASS;
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) return 1;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 520;
    int winH = 340;
    int x = (screenW - winW) / 2;
    int y = (screenH - winH) / 2;
    if (x < 0) x = 32;
    if (y < 0) y = 32;

    HWND hMain = CreateWindowExW(
        0,
        INSTALLER_WIN_CLASS,
        g_appName,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, winW, winH,
        NULL, NULL, hInstance, NULL);
    if (!hMain) return 1;

    // --- 安装按钮 ---
    {
        HWND btn = CreateWindowExW(
            0,
            L"BUTTON",
            L"快速安装",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            180, 210, 160, 48,
            hMain, (HMENU)INSTALL_BUTTON_ID, hInstance, NULL);
        if (btn) {
            HFONT hF = CreateFontW(
                22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
            if (hF) SendMessageW(btn, WM_SETFONT, (WPARAM)hF, TRUE);
        }
    }

    // --- 状态标签 ---
    {
        HWND lbl = CreateWindowExW(
            0, L"STATIC",
            L"点击上方按钮开始安装",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            40, 270, 440, 24,
            hMain, (HMENU)1002, hInstance, NULL);
        if (lbl) {
            HFONT hF = CreateFontW(
                16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
            if (hF) SendMessageW(lbl, WM_SETFONT, (WPARAM)hF, TRUE);
        }
    }

    ShowWindow(hMain, nCmdShow);
    UpdateWindow(hMain);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

// ====================================================================
// 绘制安装界面（渐变背景 + 标题）
// ====================================================================
static void PaintInstaller(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    TRIVERTEX vtx[2];
    memset(&vtx, 0, sizeof(vtx));
    vtx[0].x     = rc.left;
    vtx[0].y     = rc.top;
    vtx[0].Red   = 0x15 << 8;
    vtx[0].Green = 0x1c << 8;
    vtx[0].Blue  = 0x38 << 8;
    vtx[0].Alpha = 0xFF << 8;
    vtx[1].x     = rc.right;
    vtx[1].y     = rc.bottom;
    vtx[1].Red   = 0x0a << 8;
    vtx[1].Green = 0x0e << 8;
    vtx[1].Blue  = 0x1f << 8;
    vtx[1].Alpha = 0xFF << 8;

    GRADIENT_RECT grect;
    grect.UpperLeft  = 0;
    grect.LowerRight = 1;
    GradientFill(hdc, vtx, 2, &grect, 1, GRADIENT_FILL_RECT_V);

    SetBkMode(hdc, TRANSPARENT);

    // 标题
    SetTextColor(hdc, RGB(0xE0, 0xE8, 0xFF));
    HFONT hTitle = CreateFontW(
        28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
    if (hTitle) {
        HGDIOBJ prev = SelectObject(hdc, hTitle);
        RECT tr;
        tr.left = 40; tr.top = 32; tr.right = rc.right - 40; tr.bottom = 80;
        DrawTextW(hdc, g_appName, -1, &tr, DT_LEFT | DT_TOP | DT_SINGLELINE);
        if (prev) SelectObject(hdc, prev);
        DeleteObject(hTitle);
    }

    // 副标题
    SetTextColor(hdc, RGB(0xA0, 0xB0, 0xD0));
    HFONT hSub = CreateFontW(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
    if (hSub) {
        HGDIOBJ prev = SelectObject(hdc, hSub);
        RECT tr2;
        tr2.left = 40; tr2.top = 84; tr2.right = rc.right - 40; tr2.bottom = 120;
        DrawTextW(hdc, L"一键安装，创建桌面快捷方式和开机启动项",
                  -1, &tr2, DT_LEFT | DT_TOP | DT_SINGLELINE);
        if (prev) SelectObject(hdc, prev);
        DeleteObject(hSub);
    }
}

// ====================================================================
// 安装窗口过程
// ====================================================================
LRESULT CALLBACK InstallWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintInstaller(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(0xE0, 0xE8, 0xFF));
            SetBkMode(hdcStatic, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        case WM_CTLCOLORBTN:
        {
            HDC hdcBtn = (HDC)wParam;
            SetTextColor(hdcBtn, RGB(0xFF, 0xFF, 0xFF));
            SetBkMode(hdcBtn, TRANSPARENT);
            return (LRESULT)CreateSolidBrush(RGB(0x30, 0x70, 0xD0));
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == INSTALL_BUTTON_ID &&
                HIWORD(wParam) == BN_CLICKED)
            {
                HWND btn = GetDlgItem(hwnd, INSTALL_BUTTON_ID);
                if (btn) EnableWindow(btn, FALSE);
                SetWindowTextW(GetDlgItem(hwnd, 1002), L"正在安装，请稍候...");

                BOOL ok = DoInstallStep(hwnd);

                if (ok) {
                    SetWindowTextW(GetDlgItem(hwnd, 1002),
                                  L"安装完成，程序已启动");
                    MessageBoxW(hwnd,
                        L"安装成功！\n桌面快捷方式和开机启动项已创建。",
                        g_appName, MB_ICONINFORMATION | MB_OK);
                } else {
                    SetWindowTextW(GetDlgItem(hwnd, 1002),
                                  L"安装失败，请重试");
                    if (btn) EnableWindow(btn, TRUE);
                }
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ====================================================================
// 路径拼接辅助函数
// ====================================================================
static void PathAppend(wchar_t* out, size_t outSize,
                        const wchar_t* base, const wchar_t* extra)
{
    size_t bi = 0;
    while (bi + 1 < outSize && base[bi]) {
        out[bi] = base[bi];
        bi++;
    }
    if (bi > 0 && out[bi - 1] != L'\\' && out[bi - 1] != L'/') {
        if (bi + 1 < outSize) out[bi++] = L'\\';
    }
    size_t xi = 0;
    while (bi + 1 < outSize && extra[xi]) {
        out[bi++] = extra[xi++];
    }
    out[bi] = 0;
}

// ====================================================================
// 写入注册表
// ====================================================================
static BOOL WriteRegistryValue(HKEY hKeyRoot, const wchar_t* subKey,
                             const wchar_t* valueName, const wchar_t* valueData)
{
    HKEY hKey = NULL;
    LONG rc = RegCreateKeyExW(hKeyRoot, subKey, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE,
                               NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS) return FALSE;

    size_t byteLen = 0;
    while (byteLen < 8192 && valueData[byteLen]) byteLen++;
    byteLen = (byteLen + 1) * sizeof(wchar_t);

    LONG wr = RegSetValueExW(hKey, valueName, 0, REG_SZ,
                             (const BYTE*)valueData, (DWORD)byteLen);
    RegCloseKey(hKey);
    return wr == ERROR_SUCCESS;
}

// ====================================================================
// 创建桌面快捷方式
// ====================================================================
static BOOL CreateShortcutAt(const wchar_t* targetPath,
                              const wchar_t* workingDir,
                              const wchar_t* lnkPath)
{
    HRESULT hr;
    IShellLinkW* psl = NULL;
    IPersistFile* ppf = NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLinkW, (void**)&psl);
    if (FAILED(hr) || psl == NULL) {
        CoUninitialize();
        return FALSE;
    }

    psl->SetPath(targetPath);
    psl->SetWorkingDirectory(workingDir);
    psl->SetIconLocation(targetPath, 0);
    psl->SetDescription(g_appName);

    hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
    if (FAILED(hr) || ppf == NULL) {
        psl->Release();
        CoUninitialize();
        return FALSE;
    }

    hr = ppf->Save(lnkPath, TRUE);
    ppf->Release();
    psl->Release();
    CoUninitialize();
    return SUCCEEDED(hr);
}

// ====================================================================
// 从 PE 资源中提取嵌入的 app.exe
// ====================================================================
static BOOL ExtractResourceFile(UINT resourceId, const wchar_t* targetPath)
{
    HMODULE hMod = GetModuleHandleW(NULL);
    if (hMod == NULL) return FALSE;

    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(resourceId), L"BINARY");
    if (hRes == NULL) {
        hRes = FindResourceW(hMod, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    }
    if (hRes == NULL) return FALSE;

    DWORD size = SizeofResource(hMod, hRes);
    if (size == 0) return FALSE;

    HGLOBAL hGlobal = LoadResource(hMod, hRes);
    if (hGlobal == NULL) return FALSE;

    const void* data = LockResource(hGlobal);
    if (data == NULL) return FALSE;

    HANDLE hFile = CreateFileW(targetPath, GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);

    if (!ok || written != size) {
        DeleteFileW(targetPath);
        return FALSE;
    }
    return TRUE;
}

// ====================================================================
// 执行安装步骤
// ====================================================================
BOOL DoInstallStep(HWND hwnd)
{
    wchar_t installDir[1024];
    wchar_t targetPath[1024];

    memset(installDir, 0, sizeof(installDir));
    memset(targetPath, 0, sizeof(targetPath));

    // --- 步骤 1：选择安装目录 ---
    {
        DWORD sz = GetEnvironmentVariableW(L"LocalAppData", installDir, 900);
        if (sz == 0 || sz >= 900) {
            GetCurrentDirectoryW(900, installDir);
        }

        PathAppend(installDir, sizeof(installDir) / sizeof(installDir[0]),
                  installDir, L"ChouRen");
    }

    // --- 步骤 2：创建安装目录 ---
    if (!CreateDirectoryW(installDir, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            MessageBoxW(hwnd, L"无法创建安装目录", g_appName,
                         MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }

    // --- 步骤 3：提取 app.exe（先尝试资源，再尝试同目录文件） ---
    PathAppend(targetPath, sizeof(targetPath) / sizeof(targetPath[0]),
              installDir, L"app.exe");

    if (!ExtractResourceFile(101, targetPath)) {
        wchar_t currentDir[1024];
        wchar_t srcPath[1024];
        memset(currentDir, 0, sizeof(currentDir));
        GetModuleFileNameW(NULL, currentDir, 1000);
        int slashAt = -1;
        for (int k = 0; currentDir[k]; k++)
            if (currentDir[k] == L'\\') slashAt = k;
        if (slashAt > 0) currentDir[slashAt + 1] = 0;

        PathAppend(srcPath, sizeof(srcPath) / sizeof(srcPath[0]),
                  currentDir, L"app.exe");

        if (!CopyFileW(srcPath, targetPath, FALSE)) {
            MessageBoxW(hwnd,
                L"找不到主程序 app.exe，请确保 app.exe 与安装程序在同一目录。",
                g_appName, MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }

    // --- 步骤 4：注册表注册 ---
    {
        WriteRegistryValue(HKEY_CURRENT_USER, g_regKeyName,
                             L"DisplayName", g_appName);
        WriteRegistryValue(HKEY_CURRENT_USER, g_regKeyName,
                             L"InstallLocation", installDir);
        WriteRegistryValue(HKEY_CURRENT_USER, g_regKeyName,
                             L"TargetUrl", g_targetUrl);

        // 添加到卸载列表（可选）
        wchar_t uninstallKey[512];
        int i;
        const wchar_t* uk = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ChouRen";
        for (i = 0; i < 510 && uk[i]; i++) uninstallKey[i] = uk[i];
        uninstallKey[i] = 0;

        WriteRegistryValue(HKEY_CURRENT_USER, uninstallKey,
                             L"DisplayName", g_appName);
        WriteRegistryValue(HKEY_CURRENT_USER, uninstallKey,
                             L"DisplayIcon", targetPath);
        WriteRegistryValue(HKEY_CURRENT_USER, uninstallKey,
                             L"InstallLocation", installDir);
    }

    // --- 步骤 5：创建桌面快捷方式（"抽人软件.lnk" ---
    {
        wchar_t desktop[MAX_PATH];
        wchar_t lnkPath[MAX_PATH];
        memset(desktop, 0, sizeof(desktop));
        memset(lnkPath, 0, sizeof(lnkPath));

        if (!SHGetSpecialFolderPathW(NULL, desktop, CSIDL_DESKTOPDIRECTORY, FALSE)) {
            DWORD sz = GetEnvironmentVariableW(L"USERPROFILE", desktop, MAX_PATH - 16);
            if (sz == 0 || sz >= MAX_PATH - 16) desktop[0] = 0;
            int last = 0;
            for (int k = 0; desktop[k]; k++) last = k;
            if (last > 0) {
                desktop[last + 1] = L'D'; desktop[last + 2] = L'e';
                desktop[last + 3] = L's'; desktop[last + 4] = L'k';
                desktop[last + 5] = L't'; desktop[last + 6] = L'o';
                desktop[last + 7] = L'p'; desktop[last + 8] = 0;
            }
        }

        PathAppend(lnkPath, sizeof(lnkPath) / sizeof(lnkPath[0]),
                  desktop, g_shortcutName);

        if (!CreateShortcutAt(targetPath, installDir, lnkPath)) {
            // 快捷方式创建失败，但不影响整体安装
        }
    }

    // --- 步骤 6：创建开机启动项（启动文件夹中的快捷方式） ---
    {
        wchar_t startup[MAX_PATH];
        wchar_t lnkPath[MAX_PATH];
        memset(startup, 0, sizeof(startup));
        memset(lnkPath, 0, sizeof(lnkPath));

        if (SHGetSpecialFolderPathW(NULL, startup, CSIDL_STARTUP, FALSE)) {
            PathAppend(lnkPath, sizeof(lnkPath) / sizeof(lnkPath[0]),
                      startup, g_shortcutName);
            CreateShortcutAt(targetPath, installDir, lnkPath);
        }
    }

    // --- 步骤 7：启动主程序 ---
    {
        SHELLEXECUTEINFOW sei;
        memset(&sei, 0, sizeof(sei));
        sei.cbSize     = sizeof(sei);
        sei.fMask      = SEE_MASK_FLAG_NO_UI;
        sei.hwnd       = hwnd;
        sei.lpVerb     = L"open";
        sei.lpFile     = targetPath;
        sei.nShow      = SW_SHOWNORMAL;
        ShellExecuteExW(&sei);
    }

    return TRUE;
}
