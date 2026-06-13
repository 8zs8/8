#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdlib.h>
#include <wchar.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "comdlg32.lib")

#define ICON_RESOURCE_ID   1
#define ID_BTN_INSTALL     2001
#define ID_STATIC_TITLE    2002
#define ID_STATIC_SUB      2003
#define ID_STATIC_STATUS   2004
#define ID_BTN_CLOSE       2005

#define RES_ID_APP_EXE     1001
#define RES_ID_APP_ICO     1002

#define INSTALLER_CLASS    L"InjectorWindowClass"
#define APP_REG_NAME       L"QuickWebLauncher"
#define APP_EXE_NAME       L"app.exe"
#define APP_ICO_NAME       L"app.ico"
/* shortcut names - ASCII only to avoid encoding issues */
#define DESKTOP_LNK_NAME   L"QuickWebLauncher.lnk"
#define STARTUP_LNK_NAME   L"QuickWebLauncher.lnk"

static HWND g_hBtn    = NULL;
static HWND g_hStatus = NULL;
static HWND g_hWnd    = NULL;

static LPCWSTR kUrl = L"https://8zs8.github.io/8/";

/* forward declarations */
LRESULT CALLBACK InstallerWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    DoInstall(VOID);
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
void    ShowInstallCompleteUI(VOID);

/* ================================================================ */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;

    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = InstallerWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(15, 23, 42));
    wc.lpszClassName = INSTALLER_CLASS;
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    wc.style         = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(NULL, L"Failed to register window class", L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    int w = 560, h = 380;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_hWnd = CreateWindowExW(
        WS_EX_WINDOWEDGE | WS_EX_LAYERED,
        INSTALLER_CLASS,
        L"Quick Install",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        sx, sy, w, h,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!g_hWnd)
    {
        MessageBoxW(NULL, L"Failed to create window", L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    SetLayeredWindowAttributes(g_hWnd, 0, (BYTE)255, LWA_ALPHA);

    SendMessageW(g_hWnd, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));
    SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

/* ================================================================
   Window procedure
   ================================================================ */
LRESULT CALLBACK InstallerWndProc(HWND hWnd, UINT message,
                                  WPARAM wParam, LPARAM lParam)
{
    static HFONT s_hTitleFont  = NULL;
    static HFONT s_hSubFont    = NULL;
    static HFONT s_hMidFont    = NULL;
    static HFONT s_hBtnFont    = NULL;
    static HFONT s_hStatusFont = NULL;

    switch (message)
    {
        case WM_CREATE:
        {
            HINSTANCE hInst = ((LPCREATESTRUCTW)lParam)->hInstance;

            CreateWindowW(L"STATIC", L"Welcome",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 40, 480, 48,
                          hWnd, (HMENU)ID_STATIC_TITLE, hInst, NULL);

            CreateWindowW(L"STATIC", L"One-click install, instant access",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 96, 480, 28,
                          hWnd, (HMENU)ID_STATIC_SUB, hInst, NULL);

            CreateWindowW(L"STATIC", L"Quick Web Launcher",
                          WS_VISIBLE | WS_CHILD | SS_CENTER,
                          40, 150, 480, 36,
                          hWnd, NULL, hInst, NULL);

            g_hBtn = CreateWindowW(L"BUTTON", L"[ Install Now ]",
                                   WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                                   170, 220, 220, 56,
                                   hWnd, (HMENU)ID_BTN_INSTALL, hInst, NULL);

            g_hStatus = CreateWindowW(L"STATIC",
                                      L"Click the button above to install",
                                      WS_VISIBLE | WS_CHILD | SS_CENTER,
                                      40, 300, 480, 24,
                                      hWnd, (HMENU)ID_STATIC_STATUS, hInst, NULL);

            /* fonts */
            s_hTitleFont  = CreateFontW(36, 0, 0, 0, FW_BOLD,     FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hSubFont    = CreateFontW(18, 0, 0, 0, FW_NORMAL,   FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hMidFont    = CreateFontW(28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hBtnFont    = CreateFontW(20, 0, 0, 0, FW_BOLD,     FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hStatusFont = CreateFontW(16, 0, 0, 0, FW_NORMAL,   FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            /* assign fonts by ID */
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
                    if (unassignedIdx == 0)
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
            GetWindowTextW(lp->hwndItem, text, 256);
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
                SetStatusText(L"Installing, please wait...");
                EnableWindow(g_hBtn, FALSE);
                UpdateWindow(g_hStatus);

                if (DoInstall())
                    ShowInstallCompleteUI();
                else
                {
                    SetStatusText(L"Install failed. Try running as administrator.");
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

/* ================================================================
   Helper: set status bar text
   ================================================================ */
void SetStatusText(LPCWSTR text)
{
    if (g_hStatus)
    {
        SetWindowTextW(g_hStatus, text);
        UpdateWindow(g_hStatus);
    }
}

/* ================================================================
   Helper: ensure directory exists (create parents recursively)
   ================================================================ */
BOOL EnsureDirExists(LPCWSTR path)
{
    if (PathFileExistsW(path)) return TRUE;

    WCHAR parent[MAX_PATH];
    wcsncpy(parent, path, MAX_PATH - 1);
    parent[MAX_PATH - 1] = L'\0';
    PathRemoveFileSpecW(parent);

    if (wcslen(parent) == 0) return FALSE;
    if (!PathFileExistsW(parent) && !EnsureDirExists(parent))
        return FALSE;

    return CreateDirectoryW(path, NULL)
        || GetLastError() == ERROR_ALREADY_EXISTS;
}

/* ================================================================
   Extract binary from PE resource -> write to targetPath
   ================================================================ */
BOOL ExtractResourceFile(UINT resourceId, LPCWSTR resourceType,
                         LPCWSTR targetPath)
{
    HMODULE hModule = GetModuleHandleW(NULL);
    if (!hModule) return FALSE;

    HRSRC hResInfo = FindResourceW(hModule,
                                   MAKEINTRESOURCEW(resourceId),
                                   resourceType);
    if (!hResInfo) return FALSE;

    HGLOBAL hResData = LoadResource(hModule, hResInfo);
    if (!hResData) return FALSE;

    DWORD  size  = SizeofResource(hModule, hResInfo);
    LPVOID pData = LockResource(hResData);
    if (!pData || size == 0) return FALSE;

    DeleteFileW(targetPath);

    HANDLE hFile = CreateFileW(targetPath, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, pData, size, &written, NULL);
    CloseHandle(hFile);

    if (!ok || written != size)
    {
        DeleteFileW(targetPath);
        return FALSE;
    }
    return TRUE;
}

/* ================================================================
   Register components: registry entries + shortcuts + startup
   ================================================================ */
BOOL RegisterComponents(LPCWSTR installDir, LPCWSTR exePath)
{
    SetStatusText(L"Registering components...");
    if (!CreateRegistryEntries(installDir, exePath))
        return FALSE;

    SetStatusText(L"Creating desktop shortcut...");
    CreateDesktopShortcut(exePath);

    SetStatusText(L"Setting up auto-start...");
    CreateStartupShortcut(exePath);

    return TRUE;
}

/* ================================================================
   Main install flow: extract -> register -> launch
   ================================================================ */
BOOL DoInstall(VOID)
{
    /* 1. Determine install directory */
    WCHAR installDir[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL,
                                    SHGFP_TYPE_CURRENT, installDir)))
    {
        wcscpy(installDir, L"C:\\Program Files");
    }
    PathAppendW(installDir, L"QuickWebLauncher");

    if (!EnsureDirExists(installDir))
    {
        WCHAR local[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                       SHGFP_TYPE_CURRENT, local)))
        {
            wcscpy(installDir, local);
            PathAppendW(installDir, L"QuickWebLauncher");
            if (!EnsureDirExists(installDir)) return FALSE;
        }
        else
            return FALSE;
    }

    /* 2. Extract main program from resource */
    SetStatusText(L"Extracting main program...");

    WCHAR dstExe[MAX_PATH];
    wcscpy(dstExe, installDir);
    PathAppendW(dstExe, APP_EXE_NAME);

    if (!ExtractResourceFile(RES_ID_APP_EXE, RT_RCDATA, dstExe))
    {
        WCHAR msg[512];
        wsprintfW(msg, L"Failed to extract: %s", dstExe);
        MessageBoxW(g_hWnd, msg, L"Install Error", MB_ICONERROR | MB_OK);
        return FALSE;
    }

    /* extract icon (non-fatal if it fails) */
    WCHAR dstIcon[MAX_PATH];
    wcscpy(dstIcon, installDir);
    PathAppendW(dstIcon, APP_ICO_NAME);
    ExtractResourceFile(RES_ID_APP_ICO, RT_RCDATA, dstIcon);

    /* 3. Register components */
    if (!RegisterComponents(installDir, dstExe))
    {
        MessageBoxW(g_hWnd, L"Component registration failed", L"Install Error",
                    MB_ICONERROR | MB_OK);
        return FALSE;
    }

    /* 4. Launch main program */
    SetStatusText(L"Launching application...");
    INT_PTR res = (INT_PTR)ShellExecuteW(NULL, L"open", dstExe,
                                         NULL, installDir, SW_SHOWNORMAL);
    if (res <= 32)
    {
        WCHAR msg[512];
        wsprintfW(msg, L"Install complete. Could not start app (error %ld). "
                        L"Please double-click the desktop shortcut manually.",
                        (LONG)res);
        MessageBoxW(g_hWnd, msg, L"Done", MB_ICONINFORMATION | MB_OK);
    }

    return TRUE;
}

/* ================================================================
   Write registry entries (uninstall info + app info)
   ================================================================ */
BOOL CreateRegistryEntries(LPCWSTR installDir, LPCWSTR exePath)
{
    WCHAR regPath[256];
    wsprintfW(regPath,
              L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s",
              APP_REG_NAME);

    HKEY hRoot = HKEY_LOCAL_MACHINE;
    HKEY hKey  = NULL;
    if (RegCreateKeyExW(hRoot, regPath, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                        &hKey, NULL) != ERROR_SUCCESS)
    {
        hRoot = HKEY_CURRENT_USER;
        if (RegCreateKeyExW(hRoot, regPath, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                            &hKey, NULL) != ERROR_SUCCESS)
            return FALSE;
    }

    WCHAR displayIcon[MAX_PATH];
    wsprintfW(displayIcon, L"%s,0", exePath);

    RegSetValueExW(hKey, L"DisplayName",      0, REG_SZ,
                   (BYTE*)L"QuickWebLauncher",
                   (DWORD)(wcslen(L"QuickWebLauncher") + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"DisplayIcon",      0, REG_SZ,
                   (BYTE*)displayIcon,
                   (DWORD)(wcslen(displayIcon) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"DisplayVersion",   0, REG_SZ,
                   (BYTE*)L"1.0.0",
                   (DWORD)(wcslen(L"1.0.0") + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"InstallLocation",  0, REG_SZ,
                   (BYTE*)installDir,
                   (DWORD)(wcslen(installDir) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"Publisher",        0, REG_SZ,
                   (BYTE*)L"8zs8",
                   (DWORD)(wcslen(L"8zs8") + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"URLInfoAbout",     0, REG_SZ,
                   (BYTE*)kUrl,
                   (DWORD)(wcslen(kUrl) + 1) * sizeof(WCHAR));

    DWORD sizeMB = 1;
    RegSetValueExW(hKey, L"EstimatedSize", 0, REG_DWORD,
                   (BYTE*)&sizeMB, sizeof(DWORD));

    RegCloseKey(hKey);

    /* app info under HKCU */
    WCHAR appKey[256];
    wsprintfW(appKey, L"Software\\%s", APP_REG_NAME);
    HKEY hkApp = NULL;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, appKey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                        &hkApp, NULL) == ERROR_SUCCESS)
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

/* ================================================================
   Shortcut creation via IShellLinkW
   ================================================================ */
BOOL CreateShortcut(LPCWSTR targetPath, LPCWSTR shortcutFolder, LPCWSTR name)
{
    WCHAR lnkPath[MAX_PATH];
    wcscpy(lnkPath, shortcutFolder);
    PathAppendW(lnkPath, name);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    IShellLinkW* psl = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
    if (FAILED(hr)) return FALSE;

    psl->SetPath(targetPath);

    WCHAR workDir[MAX_PATH];
    wcscpy(workDir, targetPath);
    PathRemoveFileSpecW(workDir);
    psl->SetWorkingDirectory(workDir);

    psl->SetDescription(L"QuickWebLauncher");
    psl->SetIconLocation(targetPath, 0);

    IPersistFile* ppf = NULL;
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
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL,
                                    SHGFP_TYPE_CURRENT, desk)))
        return FALSE;
    return CreateShortcut(targetPath, desk, DESKTOP_LNK_NAME);
}

BOOL CreateStartupShortcut(LPCWSTR targetPath)
{
    WCHAR startup[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL,
                                    SHGFP_TYPE_CURRENT, startup)))
        return FALSE;
    return CreateShortcut(targetPath, startup, STARTUP_LNK_NAME);
}

/* ================================================================
   Post-install UI update
   ================================================================ */
void ShowInstallCompleteUI(VOID)
{
    SetStatusText(L"Done! Desktop shortcut created.");
    if (g_hBtn)
    {
        SetWindowTextW(g_hBtn, L"[ Done ]");
        SetWindowLongPtrW(g_hBtn, GWLP_ID, ID_BTN_CLOSE);
        EnableWindow(g_hBtn, TRUE);
        InvalidateRect(g_hBtn, NULL, TRUE);
    }
}
