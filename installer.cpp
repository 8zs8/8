// installer.cpp - One-click installer for Quick Web Launcher
// ASCII-only, works on any Windows codepage.
// Reads app.exe from internal resource, copies it to install folder,
// writes registry entries for "Add/Remove Programs", creates desktop
// shortcut "Quick Web Launcher.lnk", and registers it for autostart.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "comctl32.lib")

#define ICON_RESOURCE_ID        1
#define RES_ID_APP_EXE          1001
#define INSTALLER_CLASS         L"QWLInstallerClass"
#define APP_REG_NAME            L"QuickWebLauncher"
#define APP_EXE_FILENAME        L"app.exe"
#define APP_LNK_NAME            L"Quick Web Launcher.lnk"

#define INSTALL_BTN_ID          5001
#define STATUS_LABEL_ID        5002

static HWND g_hInstallBtn   = NULL;
static HWND g_hStatusLabel = NULL;
static HWND g_hWnd         = NULL;

static const wchar_t* k_target_url =
    L"https://8zs8.github.io/8/";

// ----------------- forward decls --------------------
LRESULT CALLBACK InstallerWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    DoInstallStep(void);
BOOL    ExtractAppExe(const wchar_t* dst);
BOOL    WriteRegistry(const wchar_t* install_dir,
                        const wchar_t* exe_path);
BOOL    CreateDesktopLink(const wchar_t* target);
BOOL    CreateStartupLink(const wchar_t* target);
void    SetStatus(const wchar_t* text);
void    SetButton(const wchar_t* text);

// ============================================================
//  WinMain
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc     = InstallerWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(20, 36, 70));
    wc.lpszClassName = INSTALLER_CLASS;
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);

    int w = 520;
    int h = 340;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_hWnd = CreateWindowExW(
        WS_EX_WINDOWEDGE,
        INSTALLER_CLASS,
        L"Quick Web Launcher - Install",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        sx, sy, w, h,
        NULL, NULL, hInstance, NULL);

    if (!g_hWnd) {
        MessageBoxW(NULL, L"Failed to create installer window",
                  L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    SendMessageW(g_hWnd, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadIconW(hInstance,
                                   MAKEINTRESOURCEW(ICON_RESOURCE_ID)));
    SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadIconW(hInstance,
                                   MAKEINTRESOURCEW(ICON_RESOURCE_ID)));

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

// ============================================================
//  Window proc
// ============================================================
LRESULT CALLBACK InstallerWndProc(HWND hwnd, UINT msg,
                                   WPARAM wParam, LPARAM lParam)
{
    static HFONT s_hTitle = NULL;
    static HFONT s_hMid = NULL;
    static HFONT s_hBtn = NULL;
    static HFONT s_hStatus = NULL;

    switch (msg)
    {
        case WM_CREATE:
        {
            HINSTANCE hInst = ((LPCREATESTRUCTW)lParam)->hInstance;

            // title line "Quick Web Launcher"
            CreateWindowW(L"STATIC", L"Quick Web Launcher",
                        WS_VISIBLE | WS_CHILD | SS_CENTER,
                        40, 30, 440, 44,
                        hwnd, (HMENU)1001, hInst, NULL);

            // subtitle "click the button to install"
            CreateWindowW(L"STATIC",
                        L"Click the button below to install - the program will"
                        L" appear in the system tray with a top-most floating "
                        L"button.",
                        WS_VISIBLE | WS_CHILD | SS_CENTER,
                        40, 88, 440, 50,
                        hwnd, (HMENU)1002, hInst, NULL);

            // Install button
            g_hInstallBtn = CreateWindowW(
                L"BUTTON", L"[ Install Now ]",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                180, 170, 160, 52,
                hwnd, (HMENU)INSTALL_BTN_ID, hInst, NULL);

            // status label
            g_hStatusLabel = CreateWindowW(
                L"STATIC", L"Ready.",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                40, 250, 440, 28,
                hwnd, (HMENU)STATUS_LABEL_ID, hInst, NULL);

            // fonts (using system default font if create fails)
            s_hTitle  = CreateFontW(34, 0, 0, 0, FW_BOLD,
                          FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                          L"Segoe UI");
            s_hMid    = CreateFontW(24, 0, 0, 0, FW_SEMIBOLD,
                          FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                          L"Segoe UI");
            s_hBtn    = CreateFontW(20, 0, 0, 0, FW_BOLD,
                          FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                          L"Segoe UI");
            s_hStatus = CreateFontW(16, 0, 0, 0, FW_NORMAL,
                          FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                          L"Segoe UI");

            // Assign fonts to child windows by enum
            HWND ch = GetWindow(hwnd, GW_CHILD);
            int idx = 0;
            while (ch) {
                LONG_PTR id = GetWindowLongPtrW(ch, GWLP_ID);
                if (id == 1001)
                    SendMessageW(ch, WM_SETFONT, (WPARAM)s_hTitle, TRUE);
                else if (id == 1002)
                    SendMessageW(ch, WM_SETFONT, (WPARAM)s_hStatus, TRUE);
                else if (id == INSTALL_BTN_ID)
                    SendMessageW(ch, WM_SETFONT, (WPARAM)s_hBtn, TRUE);
                else if (id == STATUS_LABEL_ID)
                    SendMessageW(ch, WM_SETFONT, (WPARAM)s_hStatus, TRUE);
                ch = GetWindow(ch, GW_HWNDNEXT);
                idx++;
            }
            return 0;
        }

        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(245, 248, 255));
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == INSTALL_BTN_ID) {
                SetStatus(L"Installing - please wait...");
                EnableWindow(g_hInstallBtn, FALSE);
                UpdateWindow(g_hStatusLabel);
                BOOL ok = DoInstallStep();
                if (ok) {
                    SetStatus(L"Installation completed successfully.");
                    SetButton(L"Close");
                    EnableWindow(g_hInstallBtn, TRUE);
                }
                else {
                    SetStatus(L"Installation failed. Run as administrator?");
                    EnableWindow(g_hInstallBtn, TRUE);
                }
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            if (s_hTitle)  DeleteObject(s_hTitle);
            if (s_hMid)    DeleteObject(s_hMid);
            if (s_hBtn)    DeleteObject(s_hBtn);
            if (s_hStatus) DeleteObject(s_hStatus);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void SetStatus(const wchar_t* text)
{
    if (g_hStatusLabel) SetWindowTextW(g_hStatusLabel, text);
}

void SetButton(const wchar_t* text)
{
    if (g_hInstallBtn) SetWindowTextW(g_hInstallBtn, text);
}

// ============================================================
//  Run all install steps
// ============================================================
BOOL DoInstallStep(void)
{
    wchar_t install_dir[MAX_PATH];
    wchar_t exe_path[MAX_PATH];

    // Decide install directory: prefer Program Files, fall back to APPDATA.
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES,
                                  NULL, 0, install_dir)))
    {
        wcscat_s(install_dir, _countof(install_dir), L"\\QuickWebLauncher");
    }
    else
    {
        install_dir[0] = 0;
    }

    if (!CreateDirectoryW(install_dir, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        // fall back to user profile
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA,
                                      NULL, 0, install_dir)))
        {
            wcscat_s(install_dir, _countof(install_dir),
                     L"\\QuickWebLauncher");
            if (!CreateDirectoryW(install_dir, NULL) &&
                GetLastError() != ERROR_ALREADY_EXISTS)
            {
                return FALSE;
            }
        }
        else return FALSE;
    }

    // 1) extract app.exe from resource
    {
        _snwprintf_s(exe_path, _countof(exe_path), _TRUNCATE,
                   L"%s\\%s", install_dir, APP_EXE_FILENAME);
        if (!ExtractAppExe(exe_path)) {
            wchar_t msg[512];
            _snwprintf_s(msg, _countof(msg), _TRUNCATE,
                       L"Could not extract app.exe to:\n%s", exe_path);
            MessageBoxW(NULL, msg, L"Install Error",
                      MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }

    // 2) write registry (Add/Remove Programs entry)
    if (!WriteRegistry(install_dir, exe_path))
    {
        // non-fatal - user can still use the app
    }

    // 3) desktop shortcut
    CreateDesktopLink(exe_path);

    // 4) autostart in Startup folder
    CreateStartupLink(exe_path);

    // 5) launch app
    ShellExecuteW(NULL, L"open", exe_path, NULL, install_dir, SW_SHOWNORMAL);

    return TRUE;
}

// ============================================================
//  Extract app.exe from PE resource (RT_RCDATA with id 1001)
// ============================================================
BOOL ExtractAppExe(const wchar_t* dst)
{
    HMODULE hMod = GetModuleHandleW(NULL);
    HRSRC   hRes = FindResourceW(hMod, MAKEINTRESOURCEW(RES_ID_APP_EXE),
                                RT_RCDATA);
    if (!hRes) return FALSE;
    HGLOBAL hGlobal = LoadResource(hMod, hRes);
    if (!hGlobal) return FALSE;
    DWORD  size = SizeofResource(hMod, hRes);
    LPVOID data = LockResource(hGlobal);
    if (!data || size == 0) return FALSE;

    HANDLE hFile = CreateFileW(dst, GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);
    if (!ok || written != size)
    {
        DeleteFileW(dst);
        return FALSE;
    }
    return TRUE;
}

// ============================================================
//  Write uninstall registry entry + app info
// ============================================================
BOOL WriteRegistry(const wchar_t* install_dir,
                    const wchar_t* exe_path)
{
    HKEY hKey;
    wchar_t subkey[256];
    _snwprintf_s(subkey, _countof(subkey), _TRUNCATE,
               L"Software\\Microsoft\\Windows\\CurrentVersion\\"
               L"Uninstall\\%s", APP_REG_NAME);

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                        &hKey, NULL) != ERROR_SUCCESS)
    {
        // Try user-level if admin failed
        if (RegCreateKeyExW(HKEY_CURRENT_USER, subkey, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                            &hKey, NULL) != ERROR_SUCCESS)
        {
            return FALSE;
        }
    }

    wchar_t display_icon[MAX_PATH];
    _snwprintf_s(display_icon, _countof(display_icon), _TRUNCATE,
              L"%s,0", exe_path);

    const wchar_t* disp_name = L"Quick Web Launcher";
    RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ,
                   (const BYTE*)disp_name,
                   (DWORD)((wcslen(disp_name) + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"DisplayIcon", 0, REG_SZ,
                   (const BYTE*)display_icon,
                   (DWORD)((wcslen(display_icon) + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"DisplayVersion", 0, REG_SZ,
                   (const BYTE*)L"1.0.0",
                   (DWORD)((wcslen(L"1.0.0") + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ,
                   (const BYTE*)install_dir,
                   (DWORD)((wcslen(install_dir) + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"Publisher", 0, REG_SZ,
                   (const BYTE*)L"QuickWebLauncher",
                   (DWORD)((wcslen(L"QuickWebLauncher") + 1) * sizeof(wchar_t)));
    DWORD size_mb = 1;
    RegSetValueExW(hKey, L"EstimatedSize", 0, REG_DWORD,
                   (const BYTE*)&size_mb, sizeof(DWORD));
    RegCloseKey(hKey);

    // Also remember target URL under HKCU\Software\QuickWebLauncher
    HKEY hk2;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                         L"Software\\QuickWebLauncher",
                         0, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_WRITE, NULL, &hk2, NULL) == ERROR_SUCCESS)
    {
        RegSetValueExW(hk2, L"InstallDir", 0, REG_SZ,
                       (const BYTE*)install_dir,
                       (DWORD)((wcslen(install_dir) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hk2, L"ExePath", 0, REG_SZ,
                       (const BYTE*)exe_path,
                       (DWORD)((wcslen(exe_path) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hk2, L"TargetUrl", 0, REG_SZ,
                       (const BYTE*)k_target_url,
                       (DWORD)((wcslen(k_target_url) + 1) * sizeof(wchar_t)));
        RegCloseKey(hk2);
    }
    return TRUE;
}

// ============================================================
//  Create a .lnk shortcut to target at folder
// ============================================================
static BOOL CreateShortcutAt(const wchar_t* target,
                              const wchar_t* folder,
                              const wchar_t* lnk_name)
{
    wchar_t lnk_path[MAX_PATH];
    _snwprintf_s(lnk_path, _countof(lnk_path), _TRUNCATE,
               L"%s\\%s", folder, lnk_name);

    IShellLinkW* psl = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  &IID_IShellLinkW, (LPVOID*)&psl);
    if (FAILED(hr) || !psl) return FALSE;

    psl->lpVtbl->SetPath(psl, target);

    wchar_t work_dir[MAX_PATH];
    wcsncpy_s(work_dir, _countof(work_dir), target, _TRUNCATE);
    PathRemoveFileSpecW(work_dir);
    psl->lpVtbl->SetWorkingDirectory(psl, work_dir);
    psl->lpVtbl->SetDescription(psl, L"Quick Web Launcher");
    psl->lpVtbl->SetIconLocation(psl, target, 0);
    psl->lpVtbl->SetShowCmd(psl, SW_SHOWNORMAL);

    IPersistFile* ppf = NULL;
    hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile,
                                      (LPVOID*)&ppf);
    if (SUCCEEDED(hr) && ppf)
    {
        hr = ppf->lpVtbl->Save(ppf, lnk_path, TRUE);
        ppf->lpVtbl->Release(ppf);
    }
    psl->lpVtbl->Release(psl);
    return SUCCEEDED(hr);
}

BOOL CreateDesktopLink(const wchar_t* target)
{
    wchar_t desk[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY,
                                    NULL, 0, desk)))
        return FALSE;
    return CreateShortcutAt(target, desk, APP_LNK_NAME);
}

BOOL CreateStartupLink(const wchar_t* target)
{
    wchar_t startup[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTUP,
                                    NULL, 0, startup)))
        return FALSE;
    return CreateShortcutAt(target, startup, APP_LNK_NAME);
}
