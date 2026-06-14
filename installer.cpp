// installer.cpp - Quick Web Launcher installer
// ASCII-only source. Writes app.exe from embedded resource to
// C:\Program Files\QuickWebLauncher (or under CSIDL_PROGRAM_FILES),
// creates a desktop shortcut, registers in the Add/Remove Programs
// list, and registers a startup (run-on-login) entry.
//
// Build (after app.exe is built):
//   windres -o inst_res.o installer.rc
//   g++ -O2 -mwindows -o installer.exe installer.cpp inst_res.o \
//       -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 \
//       -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32

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
#include <shlobj.h>
#include <shlwapi.h>

// required link libraries
// NOTE for Dev-C++ users: open Installer.dev (not installer.cpp) and press Ctrl+F9
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

#define INSTALLER_ICON_ID    1
#define APP_EXE_RESOURCE_ID  101
#define APP_EXE_FILENAME     L"app.exe"
#define APP_LNK_NAME         L"Quick Web Launcher.lnk"
#define APP_REG_NAME         L"QuickWebLauncher"

#define INSTALLER_WIN_CLASS  L"QWLInstallerClass"
#define ID_INSTALL_BTN       8001
#define ID_STATUS_LABEL      8002

static HWND  g_hWnd         = NULL;
static HWND  g_hBtn         = NULL;
static HWND  g_hStatus      = NULL;
static const WCHAR g_target_url[] =
    L"https://8zs8.github.io/8/";

// ---- forward declarations ----
LRESULT CALLBACK InstallerWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL DoInstallSteps(void);
BOOL ExtractAppExeTo(const WCHAR* dst_path);
BOOL WriteRegistryEntries(const WCHAR* install_dir,
                           const WCHAR* exe_path);
BOOL CreateDesktopLink(const WCHAR* target);
BOOL CreateStartupLink(const WCHAR* target);
void SetStatusText(const WCHAR* text);

// ---- small helper: append path with backslash safety ----
static void AppendPath(WCHAR* dst, size_t dst_cch,
                        const WCHAR* folder, const WCHAR* file)
{
    // copy folder, then append file, never exceed dst_cch
    size_t i = 0;
    while (folder[i] && i < dst_cch - 2) {
        dst[i] = folder[i];
        i++;
    }
    // ensure trailing backslash
    if (i > 0 && dst[i - 1] != L'\\' && i < dst_cch - 1) {
        dst[i] = L'\\';
        i++;
    }
    // append file
    size_t j = 0;
    while (file[j] && i < dst_cch - 1) {
        dst[i] = file[j];
        i++;
        j++;
    }
    dst[i] = 0;
}

// ---- small helper: copy string with length limit ----
static void CopyString(WCHAR* dst, size_t dst_cch,
                        const WCHAR* src)
{
    size_t i;
    for (i = 0; i < dst_cch - 1 && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = 0;
}

// ============================================================
//  WinMain
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // init COM (needed for creating shell links / shortcuts)
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
                          COINIT_DISABLE_OLE1DDE);

    // register installer window class
    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc     = InstallerWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = INSTALLER_WIN_CLASS;
        wc.hIcon         = LoadIconW(hInstance,
                            MAKEINTRESOURCEW(INSTALLER_ICON_ID));
        if (!wc.hIcon) wc.hIcon = LoadIconW(NULL,
                            (LPCWSTR)IDI_APPLICATION);
        wc.hIconSm       = wc.hIcon;

        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                MessageBoxW(NULL, L"Could not register installer window.",
                          L"Fatal", MB_ICONERROR | MB_OK);
                CoUninitialize();
                return 1;
            }
        }
    }

    // create installer window
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 520;
    int winH = 340;
    int x    = (screenW - winW) / 2;
    int y    = (screenH - winH) / 2;

    g_hWnd = CreateWindowExW(
        WS_EX_WINDOWEDGE | WS_EX_TOPMOST,
        INSTALLER_WIN_CLASS,
        L"Quick Web Launcher Installer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, winW, winH,
        NULL, NULL, hInstance, NULL);

    if (!g_hWnd) {
        MessageBoxW(NULL, L"Failed to create installer window.",
                  L"Fatal", MB_ICONERROR | MB_OK);
        CoUninitialize();
        return 1;
    }

    // set big icon and small icon (for taskbar/alt-tab)
    HICON bigIcon = LoadIconW(hInstance,
                              MAKEINTRESOURCEW(INSTALLER_ICON_ID));
    if (!bigIcon) bigIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    SendMessageW(g_hWnd, WM_SETICON, ICON_BIG,   (LPARAM)bigIcon);
    SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)bigIcon);

    // create child controls
    // title label
    CreateWindowW(L"STATIC", L"Quick Web Launcher",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                20, 20, 460, 40,
                g_hWnd, (HMENU)1001, hInstance, NULL);

    // "description"
    CreateWindowW(L"STATIC",
                L"Click the button below to install."
                L" A top-most floating button and a system tray icon"
                L" will be added to your desktop.",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                30, 80, 440, 60,
                g_hWnd, (HMENU)1002, hInstance, NULL);

    // big Install button
    g_hBtn = CreateWindowW(
        L"BUTTON", L"Install",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
        160, 170, 200, 56,
        g_hWnd, (HMENU)ID_INSTALL_BTN, hInstance, NULL);

    // status label
    g_hStatus = CreateWindowW(
        L"STATIC", L"Ready.",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        30, 240, 440, 30,
        g_hWnd, (HMENU)ID_STATUS_LABEL, hInstance, NULL);

    // apply default system font to all children (looks decent)
    HFONT hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessageW(GetDlgItem(g_hWnd, 1001), WM_SETFONT,
                 (WPARAM)hf, TRUE);
    SendMessageW(GetDlgItem(g_hWnd, 1002), WM_SETFONT,
                 (WPARAM)hf, TRUE);
    SendMessageW(g_hBtn, WM_SETFONT, (WPARAM)hf, TRUE);
    SendMessageW(g_hStatus, WM_SETFONT, (WPARAM)hf, TRUE);

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}

// ============================================================
//  Set installer status text
// ============================================================
void SetStatusText(const WCHAR* text)
{
    if (g_hStatus) SetWindowTextW(g_hStatus, text);
}

// ============================================================
//  Installer window procedure
// ============================================================
LRESULT CALLBACK InstallerWndProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            TRIVERTEX vtx[2];
            memset(&vtx, 0, sizeof(vtx));
            vtx[0].x     = rc.left;
            vtx[0].y     = rc.top;
            vtx[0].Red   = 0x30 << 8;
            vtx[0].Green = 0x60 << 8;
            vtx[0].Blue  = 0xD0 << 8;
            vtx[0].Alpha = 0xFF << 8;
            vtx[1].x     = rc.right;
            vtx[1].y     = rc.bottom;
            vtx[1].Red   = 0x05 << 8;
            vtx[1].Green = 0x20 << 8;
            vtx[1].Blue  = 0x55 << 8;
            vtx[1].Alpha = 0xFF << 8;

            GRADIENT_RECT grect;
            grect.UpperLeft  = 0;
            grect.LowerRight = 1;
            GradientFill(hdc, vtx, 2, &grect, 1, GRADIENT_FILL_RECT_V);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        case WM_CTLCOLORBTN:
        {
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == ID_INSTALL_BTN) {
                SetStatusText(L"Installing, please wait...");
                EnableWindow(g_hBtn, FALSE);
                UpdateWindow(g_hStatus);

                BOOL ok = DoInstallSteps();
                if (ok) {
                    SetStatusText(L"Installation completed successfully."
                                L" The program is now running.");
                    SetWindowTextW(g_hBtn, L"Close");
                    EnableWindow(g_hBtn, TRUE);
                }
                else {
                    SetStatusText(L"Installation failed. Please try again.");
                    EnableWindow(g_hBtn, TRUE);
                }
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================
//  Run all install steps (called when user clicks Install)
// ============================================================
BOOL DoInstallSteps(void)
{
    WCHAR installDir[MAX_PATH];
    WCHAR appPath[MAX_PATH];
    WCHAR tmp[MAX_PATH];
    BOOL  ok;

    installDir[0] = 0;
    appPath[0]    = 0;

    // --- Step 1: extract app.exe from resource to a temp location ---
    SetStatusText(L"Extracting program files...");
    UpdateWindow(g_hStatus);

    // build temp target path: %TEMP%\app.exe  (we'll move it later)
    {
        DWORD len = GetTempPathW(MAX_PATH, tmp);
        if (len == 0 || len >= MAX_PATH) {
            MessageBoxW(NULL, L"Cannot get TEMP path.",
                      L"Error", MB_ICONERROR | MB_OK);
            return FALSE;
        }
        // append filename
        size_t i;
        for (i = 0; i < MAX_PATH - 1 && tmp[i]; i++)
            appPath[i] = tmp[i];
        const WCHAR* fn = APP_EXE_FILENAME;
        size_t j = 0;
        while (fn[j] && i < MAX_PATH - 1) {
            appPath[i] = fn[j];
            i++; j++;
        }
        appPath[i] = 0;
    }

    ok = ExtractAppExeTo(appPath);
    if (!ok) {
        MessageBoxW(NULL, L"Failed to extract app.exe from resources.",
                  L"Error", MB_ICONERROR | MB_OK);
        return FALSE;
    }

    // --- Step 2: create install directory (try Program Files first) ---
    SetStatusText(L"Creating install directory...");
    UpdateWindow(g_hStatus);

    {
        WCHAR pf[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES,
                                        NULL, 0, pf)))
        {
            // build installDir = pf + "\QuickWebLauncher"
            size_t i;
            for (i = 0; i < MAX_PATH - 20 && pf[i]; i++)
                installDir[i] = pf[i];
            const WCHAR* sub = L"\\QuickWebLauncher";
            size_t j = 0;
            while (sub[j] && i < MAX_PATH - 1) {
                installDir[i] = sub[j];
                i++; j++;
            }
            installDir[i] = 0;

            if (!CreateDirectoryW(installDir, NULL)) {
                // if already exists, fine; otherwise fallback to APPDATA
                DWORD err = GetLastError();
                if (err != ERROR_ALREADY_EXISTS) {
                    // fallback to APPDATA\QuickWebLauncher
                    WCHAR appdata[MAX_PATH];
                    if (SUCCEEDED(SHGetFolderPathW(
                                       NULL, CSIDL_APPDATA,
                                       NULL, 0, appdata)))
                    {
                        size_t k;
                        for (k = 0; k < MAX_PATH - 20 && appdata[k]; k++)
                            installDir[k] = appdata[k];
                        const WCHAR* sub2 = L"\\QuickWebLauncher";
                        size_t m = 0;
                        while (sub2[m] && k < MAX_PATH - 1) {
                            installDir[k] = sub2[m];
                            k++; m++;
                        }
                        installDir[k] = 0;

                        if (!CreateDirectoryW(installDir, NULL) &&
                            GetLastError() != ERROR_ALREADY_EXISTS)
                        {
                            MessageBoxW(NULL,
                                      L"Cannot create install directory.",
                                      L"Error", MB_ICONERROR | MB_OK);
                            return FALSE;
                        }
                    }
                }
            }
        }
    }

    // --- Step 3: copy app.exe from tmp to installDir ---
    SetStatusText(L"Copying program file to install directory...");
    UpdateWindow(g_hStatus);

    {
        WCHAR finalAppPath[MAX_PATH];
        AppendPath(finalAppPath, MAX_PATH, installDir, APP_EXE_FILENAME);

        if (!CopyFileW(appPath, finalAppPath, FALSE)) {
            DWORD err = GetLastError();
            WCHAR msg[1024];
            // use simple wsprintf to format error info
            wsprintfW(msg,
                     L"CopyFileW failed (error=%lu).\n\nFrom: %s\nTo:   %s\n\n"
                     L"Please try running the installer as administrator.",
                     err, appPath, finalAppPath);
            MessageBoxW(NULL, msg, L"Error", MB_ICONERROR | MB_OK);
            return FALSE;
        }
        CopyString(appPath, MAX_PATH, finalAppPath);
    }

    // --- Step 4: write registry entries for Add/Remove Programs ---
    SetStatusText(L"Writing registry entries...");
    UpdateWindow(g_hStatus);

    WriteRegistryEntries(installDir, appPath);

    // --- Step 5: create desktop shortcut ---
    SetStatusText(L"Creating desktop shortcut...");
    UpdateWindow(g_hStatus);

    CreateDesktopLink(appPath);

    // --- Step 6: create startup (logon) link ---
    SetStatusText(L"Registering startup link...");
    UpdateWindow(g_hStatus);

    CreateStartupLink(appPath);

    // --- Step 7: launch app.exe ---
    SetStatusText(L"Starting program...");
    UpdateWindow(g_hStatus);

    {
        HINSTANCE r = ShellExecuteW(NULL, L"open", appPath,
                                     NULL, installDir, SW_SHOWNORMAL);
        if (!r || (INT_PTR)r <= 32) {
            MessageBoxW(NULL,
                      L"Program installed but could not be auto-launched.",
                      L"Warning", MB_ICONWARNING | MB_OK);
        }
    }

    // cleanup temp
    DeleteFileW(tmp);

    return TRUE;
}

// ============================================================
//  Extract app.exe from PE resource (RCDATA/101) to dst_path
// ============================================================
BOOL ExtractAppExeTo(const WCHAR* dst_path)
{
    HMODULE hMod = GetModuleHandleW(NULL);
    HRSRC   hRes = FindResourceW(hMod,
                               MAKEINTRESOURCEW(APP_EXE_RESOURCE_ID),
                               RT_RCDATA);
    if (!hRes) {
        DWORD err = GetLastError();
        WCHAR msg[512];
        wsprintfW(msg, L"FindResourceW failed: %lu\n\n"
                 L"app.exe was not embedded into installer.exe.\n"
                 L"Please rebuild with the correct windres step.", err);
        MessageBoxW(NULL, msg, L"Error", MB_ICONERROR | MB_OK);
        return FALSE;
    }

    HGLOBAL hGlobal = LoadResource(hMod, hRes);
    if (!hGlobal) return FALSE;

    DWORD  size = SizeofResource(hMod, hRes);
    LPVOID data = LockResource(hGlobal);
    if (!data || size == 0) return FALSE;

    HANDLE hFile = CreateFileW(dst_path, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);
    if (!ok || written != size) {
        DeleteFileW(dst_path);
        return FALSE;
    }
    return TRUE;
}

// ============================================================
//  Write registry entries for Add/Remove Programs + app info
// ============================================================
BOOL WriteRegistryEntries(const WCHAR* install_dir,
                           const WCHAR* exe_path)
{
    HKEY  hKey = NULL;
    WCHAR subkey[MAX_PATH];
    WCHAR displayIcon[MAX_PATH];
    WCHAR displayName[128];
    WCHAR uninstallCmd[MAX_PATH * 2];

    // build subkey path
    {
        const WCHAR* prefix = L"Software\\Microsoft\\Windows\\"
                                 L"CurrentVersion\\Uninstall\\";
        size_t i;
        for (i = 0; i < MAX_PATH - 40 && prefix[i]; i++)
            subkey[i] = prefix[i];
        const WCHAR* suffix = APP_REG_NAME;
        size_t j = 0;
        while (suffix[j] && i < MAX_PATH - 1) {
            subkey[i] = suffix[j];
            i++; j++;
        }
        subkey[i] = 0;
    }

    // first try HKLM (needs admin)
    LONG res = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE,
                               NULL, &hKey, NULL);
    if (res != ERROR_SUCCESS) {
        // fallback to HKCU
        res = RegCreateKeyExW(HKEY_CURRENT_USER, subkey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE,
                              NULL, &hKey, NULL);
    }

    if (res != ERROR_SUCCESS) return FALSE;

    // build displayIcon
    {
        size_t i;
        for (i = 0; i < MAX_PATH - 4 && exe_path[i]; i++)
            displayIcon[i] = exe_path[i];
        const WCHAR* s = L",0";
        size_t j = 0;
        while (s[j] && i < MAX_PATH - 1) {
            displayIcon[i] = s[j];
            i++; j++;
        }
        displayIcon[i] = 0;
    }

    // build displayName
    CopyString(displayName, _countof(displayName),
              L"Quick Web Launcher");

    // build uninstall command:  cmd /c DEL /F /Q "exe_path"
    // (simple but functional; avoids external uninstaller)
    {
        const WCHAR* prefix = L"cmd.exe /c DEL /F /Q \"";
        size_t i = 0;
        for (i = 0; i < MAX_PATH * 2 - 20 && prefix[i]; i++)
            uninstallCmd[i] = prefix[i];
        size_t j = 0;
        while (exe_path[j] && i < MAX_PATH * 2 - 2) {
            uninstallCmd[i] = exe_path[j];
            i++; j++;
        }
        const WCHAR* suffix = L"\"";
        j = 0;
        while (suffix[j] && i < MAX_PATH * 2 - 1) {
            uninstallCmd[i] = suffix[j];
            i++; j++;
        }
        uninstallCmd[i] = 0;
    }

    // write values (use RegSetValueExW directly, never use _s functions)
    RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ,
                   (const BYTE*)displayName,
                   (DWORD)((wcslen(displayName) + 1) * sizeof(WCHAR)));
    RegSetValueExW(hKey, L"DisplayIcon", 0, REG_SZ,
                   (const BYTE*)displayIcon,
                   (DWORD)((wcslen(displayIcon) + 1) * sizeof(WCHAR)));
    RegSetValueExW(hKey, L"DisplayVersion", 0, REG_SZ,
                   (const BYTE*)L"1.0.0",
                   (DWORD)((wcslen(L"1.0.0") + 1) * sizeof(WCHAR)));
    RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ,
                   (const BYTE*)install_dir,
                   (DWORD)((wcslen(install_dir) + 1) * sizeof(WCHAR)));
    RegSetValueExW(hKey, L"Publisher", 0, REG_SZ,
                   (const BYTE*)L"QuickWebLauncher",
                   (DWORD)((wcslen(L"QuickWebLauncher") + 1) * sizeof(WCHAR)));
    RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ,
                   (const BYTE*)uninstallCmd,
                   (DWORD)((wcslen(uninstallCmd) + 1) * sizeof(WCHAR)));
    DWORD estimateMb = 1;
    RegSetValueExW(hKey, L"EstimatedSize", 0, REG_DWORD,
                   (const BYTE*)&estimateMb, sizeof(DWORD));

    RegCloseKey(hKey);

    // write a HKCU\Software\QuickWebLauncher key with app info
    {
        HKEY h2 = NULL;
        if (RegCreateKeyExW(HKEY_CURRENT_USER,
                            L"Software\\QuickWebLauncher",
                            0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_WRITE, NULL, &h2, NULL) == ERROR_SUCCESS)
        {
            RegSetValueExW(h2, L"InstallDir", 0, REG_SZ,
                           (const BYTE*)install_dir,
                           (DWORD)((wcslen(install_dir) + 1) * sizeof(WCHAR)));
            RegSetValueExW(h2, L"ExePath", 0, REG_SZ,
                           (const BYTE*)exe_path,
                           (DWORD)((wcslen(exe_path) + 1) * sizeof(WCHAR)));
            RegSetValueExW(h2, L"TargetUrl", 0, REG_SZ,
                           (const BYTE*)g_target_url,
                           (DWORD)((wcslen(g_target_url) + 1) *
                                     sizeof(WCHAR)));
            RegCloseKey(h2);
        }
    }
    return TRUE;
}

// ============================================================
//  Create a shortcut (.lnk) to target, placed at given folder
//  Uses C++ COM style calls (psl->SetPath(...), not psl->lpVtbl->...)
// ============================================================
static BOOL CreateShortcutInFolder(const WCHAR* target,
                                    const WCHAR* folder,
                                    const WCHAR* lnk_name)
{
    WCHAR  lnk_path[MAX_PATH];
    AppendPath(lnk_path, MAX_PATH, folder, lnk_name);

    IShellLinkW* psl = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IShellLinkW, (LPVOID*)&psl);
    if (FAILED(hr) || !psl) return FALSE;

    // target
    psl->SetPath(target);

    // working directory = parent directory of target
    {
        WCHAR wd[MAX_PATH];
        size_t i;
        for (i = 0; i < MAX_PATH - 1 && target[i]; i++)
            wd[i] = target[i];
        wd[i] = 0;
        // remove trailing filename component
        while (i > 0 && wd[i - 1] != L'\\' && wd[i - 1] != L'/') {
            wd[i - 1] = 0;
            i--;
        }
        if (i > 1 && wd[i - 1] == L'\\') {
            wd[i - 1] = 0;
        }
        psl->SetWorkingDirectory(wd);
    }

    psl->SetDescription(L"Quick Web Launcher");
    psl->SetIconLocation(target, 0);
    psl->SetShowCmd(SW_SHOWNORMAL);

    IPersistFile* ppf = NULL;
    hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
    if (SUCCEEDED(hr) && ppf) {
        hr = ppf->Save(lnk_path, TRUE);
        ppf->Release();
    }
    psl->Release();
    return SUCCEEDED(hr);
}

// ============================================================
//  Desktop shortcut
// ============================================================
BOOL CreateDesktopLink(const WCHAR* target)
{
    WCHAR desk[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY,
                                     NULL, 0, desk)))
        return FALSE;
    return CreateShortcutInFolder(target, desk, APP_LNK_NAME);
}

// ============================================================
//  Startup (logon) shortcut via Run registry key
// ============================================================
BOOL CreateStartupLink(const WCHAR* target)
{
    // Option A: create a .lnk in the Startup folder (more user-friendly)
    WCHAR startup[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTUP,
                                    NULL, 0, startup))) {
        return CreateShortcutInFolder(target, startup, APP_LNK_NAME);
    }

    // Option B (fallback): write to HKCU\Software\Microsoft\Windows\CurrentVersion\Run
    HKEY hKey = NULL;
    const WCHAR* subkey = L"Software\\Microsoft\\Windows\\"
                              L"CurrentVersion\\Run";
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subkey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE,
                        NULL, &hKey, NULL) != ERROR_SUCCESS)
        return FALSE;
    RegSetValueExW(hKey, APP_REG_NAME, 0, REG_SZ,
                   (const BYTE*)target,
                   (DWORD)((wcslen(target) + 1) * sizeof(WCHAR)));
    RegCloseKey(hKey);
    return TRUE;
}
