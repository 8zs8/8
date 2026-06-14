// ====================================================================
//  Quick Web Launcher - installer (installer.cpp)
//  ====================================================================
//
//  HOW TO COMPILE IN DEV-C++:
//
//  STEP 0: FIRST compile app.exe (see App.dev).
//          The installer.rc file embeds app.exe as a binary resource
//          so app.exe MUST exist before compiling the installer.
//
//  Method A: open the project file in Dev-C++:
//    1. File -> Open Project or File -> Open
//    2. Select: Installer.dev
//    3. Menu: Execute -> Compile  (or press Ctrl+F9)
//
//  Method B: if you get linker errors, manually set the options:
//    1. In Dev-C++, with the project open
//    2. Menu: Project -> Project Options -> Parameters tab
//    3. In the "Linker" box, paste this EXACT LINE:
//       -mwindows -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32
//    4. Click OK, then press Ctrl+F9
//
//  ====================================================================

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
#include <commctrl.h>

// ====================================================================
//  FORCE LINKER TO INCLUDE REQUIRED LIBRARIES
//  NOTE: #pragma comment(lib, ...) requires MinGW GCC 4.5+ with
//  linker directive support. Library names must be WITHOUT .lib suffix.
//  If your MinGW version does not support this, add -lxxx flags
//  through the Dev-C++ Project Options GUI (see file header).
// ====================================================================
#ifdef __GNUC__
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "ole32")
#pragma comment(lib, "uuid")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "kernel32")
#endif

// --- Forward declarations. -----------------------------------------
LRESULT CALLBACK InstallWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL DoInstallStep(HWND hwnd);

#define INSTALLER_WIN_CLASS L"QWLInstallerClass"
#define INSTALL_BUTTON_ID   1001

// ====================================================================
//  WinMain - entry point.
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
    wc.lpfnWndProc     = InstallWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0x14, 0x1A, 0x30));
    wc.lpszClassName = INSTALLER_WIN_CLASS;
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            MessageBoxW(NULL,
                L"Installer: RegisterClassExW failed.\n"
                L"Please compile by opening Installer.dev (not installer.cpp alone).",
                L"Error", MB_ICONERROR | MB_OK);
            return 1;
        }
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
        L"Quick Web Launcher - Setup",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, winW, winH,
        NULL, NULL, hInstance, NULL);
    if (!hMain) {
        MessageBoxW(NULL, L"Installer: CreateWindowExW failed.",
                  L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // --- Install button.
    {
        HWND btn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Install",
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

    // --- Status label.
    {
        HWND lbl = CreateWindowExW(
            0, L"STATIC",
            L"Click Install to begin.",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            40, 270, 440, 24,
            hMain, (HMENU)1002, hInstance, NULL);
        if (lbl) {
            HFONT hF = CreateFontW(
                16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
            if (hF) SendMessageW(lbl, WM_SETFONT, (WPARAM)hF, TRUE);
            SetWindowTextW(lbl, L"Click Install to begin.");
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
//  Draw the installer UI background + title.
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

    // Title.
    SetTextColor(hdc, RGB(0xE0, 0xE8, 0xFF));
    HFONT hTitle = CreateFontW(
        28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
    if (hTitle) {
        HGDIOBJ prev = SelectObject(hdc, hTitle);
        RECT tr;
        tr.left = 40; tr.top = 32; tr.right = rc.right - 40; tr.bottom = 80;
        DrawTextW(hdc, L"Quick Web Launcher", -1, &tr,
                  DT_LEFT | DT_TOP | DT_SINGLELINE);
        if (prev) SelectObject(hdc, prev);
        DeleteObject(hTitle);
    }

    // Subtitle.
    SetTextColor(hdc, RGB(0xA0, 0xB0, 0xD0));
    HFONT hSub = CreateFontW(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
    if (hSub) {
        HGDIOBJ prev = SelectObject(hdc, hSub);
        RECT tr;
        tr.left = 40; tr.top = 84; tr.right = rc.right - 40; tr.bottom = 120;
        DrawTextW(hdc, L"Installs the launcher program, a desktop shortcut,\n"
                  L"and a start-on-login entry.",
                  -1, &tr, DT_LEFT | DT_TOP);
        if (prev) SelectObject(hdc, prev);
        DeleteObject(hSub);
    }
}

// ====================================================================
//  Installer window procedure.
// ====================================================================
LRESULT CALLBACK InstallWndProc(HWND hwnd, UINT msg,
                                 WPARAM wParam, LPARAM lParam)
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
                SetWindowTextW(GetDlgItem(hwnd, 1002),
                              L"Installing - please wait...");

                BOOL ok = DoInstallStep(hwnd);

                if (ok) {
                    SetWindowTextW(GetDlgItem(hwnd, 1002),
                                  L"Install completed. Launcher is now running.");
                    MessageBoxW(hwnd,
                        L"Installation completed successfully.\n"
                        L"The launcher is running and a desktop shortcut has been created.",
                        L"Setup", MB_ICONINFORMATION | MB_OK);
                } else {
                    SetWindowTextW(GetDlgItem(hwnd, 1002),
                                  L"Installation failed. See the previous error.");
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
//  Small helper: concatenate two paths into an output buffer.
//  We avoid the non-portable wcscat_s/_snwprintf_s that older
//  MinGW versions do not provide.
// ====================================================================
static void PathAppend(wchar_t* out, size_t outSize,
                        const wchar_t* base, const wchar_t* extra)
{
    size_t bi = 0;
    while (bi + 1 < outSize && base[bi]) {
        out[bi] = base[bi];
        bi++;
    }
    // Ensure backslash separator is present.
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
//  Write a value under the "installed" registry key.
// ====================================================================
static BOOL WriteRegistry(const wchar_t* name, const wchar_t* value)
{
    HKEY hKey = NULL;
    const wchar_t* subkey = L"Software\\Quick Web Launcher";

    LONG rc = RegCreateKeyExW(HKEY_CURRENT_USER, subkey, 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE,
                               NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS) return FALSE;

    size_t byteLen = 0;
    while (byteLen < 8192 && value[byteLen]) byteLen++;
    byteLen = (byteLen + 1) * sizeof(wchar_t);

    LONG wr = RegSetValueExW(hKey, name, 0, REG_SZ,
                            (const BYTE*)value, (DWORD)byteLen);
    RegCloseKey(hKey);
    return wr == ERROR_SUCCESS;
}

// ====================================================================
//  Create a .lnk shortcut (IShellLinkW + IPersistFile).
//  targetPath  = absolute path to app.exe
//  workingDir  = directory that contains app.exe
//  lnkPath     = absolute path of the .lnk file to create
// ====================================================================
static BOOL CreateShortcutAt(const wchar_t* targetPath,
                              const wchar_t* workingDir,
                              const wchar_t* lnkPath)
{
    HRESULT hr;
    IShellLinkW* psl = NULL;
    IPersistFile* ppf = NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(
        CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, (void**)&psl);
    if (FAILED(hr) || psl == NULL) {
        CoUninitialize();
        return FALSE;
    }

    psl->SetPath(targetPath);
    psl->SetWorkingDirectory(workingDir);
    psl->SetIconLocation(targetPath, 0);
    psl->SetDescription(L"Quick Web Launcher");

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
//  Retrieve a string value from the PE resource (embedded app.exe).
//  Returns TRUE on success; the caller must LocalFree(*outBytes).
// ====================================================================
static BOOL ExtractResourceFile(UINT resourceId, const wchar_t* targetPath)
{
    HMODULE hMod = GetModuleHandleW(NULL);
    if (hMod == NULL) return FALSE;

    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(resourceId),
                               L"BINARY");
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
//  DoInstallStep - performs the whole install sequence.
//  Steps:
//    1. Pick a sensible install directory.
//       - Admin:  C:\Program Files\Quick Web Launcher\
//       - User:   %LOCALAPPDATA%\Quick Web Launcher\
//    2. Create the directory.
//    3. Extract app.exe from our PE resource into the install dir.
//    4. Write registry entries so the app appears in "Programs and Features".
//    5. Create a desktop shortcut (named "Quick Web Launcher.lnk").
//    6. Create a start-on-login entry in the Startup folder.
//    7. Launch app.exe so the user sees it running immediately.
// ====================================================================
BOOL DoInstallStep(HWND hwnd)
{
    wchar_t installDir[1024];
    wchar_t targetPath[1024];

    memset(installDir, 0, sizeof(installDir));
    memset(targetPath, 0, sizeof(targetPath));

    // --- Step 1: choose install directory.
    {
        HANDLE hToken = NULL;
        BOOL isAdmin = FALSE;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elev;
            DWORD cbSz = sizeof(elev);
            if (GetTokenInformation(hToken, TokenElevation, &elev, cbSz, &cbSz))
                if (elev.TokenIsElevated) isAdmin = TRUE;
            CloseHandle(hToken);
        }

        if (isAdmin) {
            DWORD sz = GetEnvironmentVariableW(L"ProgramFiles",
                                               installDir, 900);
            if (sz == 0 || sz >= 900) {
                installDir[0] = 0;
            }
        } else {
            DWORD sz = GetEnvironmentVariableW(L"LocalAppData",
                                               installDir, 900);
            if (sz == 0 || sz >= 900) {
                installDir[0] = 0;
            }
        }

        if (installDir[0] == 0) {
            // Absolute fallback: current directory.
            GetCurrentDirectoryW(900, installDir);
        }

        PathAppend(installDir, sizeof(installDir) / sizeof(installDir[0]),
                  installDir, L"Quick Web Launcher");
    }

    // --- Step 2: create directory.
    if (!CreateDirectoryW(installDir, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            wchar_t msg[1500];
            const wchar_t* prefix = L"Could not create install directory:\n";
            int i = 0;
            while (prefix[i]) { msg[i] = prefix[i]; i++; }
            int j = 0;
            while (i < 1400 && installDir[j]) { msg[i] = installDir[j]; i++; j++; }
            msg[i] = 0;
            MessageBoxW(hwnd, msg, L"Setup", MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }

    // --- Step 3: extract app.exe from our resource into that dir.
    PathAppend(targetPath, sizeof(targetPath) / sizeof(targetPath[0]),
              installDir, L"app.exe");

    if (!ExtractResourceFile(101, targetPath)) {
        // Resource 101 not found. Try to fall back to app.exe next to us.
        wchar_t currentExe[1024];
        memset(currentExe, 0, sizeof(currentExe));
        GetModuleFileNameW(NULL, currentExe, 1000);
        int slashAt = -1;
        for (int k = 0; currentExe[k]; k++)
            if (currentExe[k] == L'\\') slashAt = k;
        if (slashAt > 0) currentExe[slashAt + 1] = 0;

        wchar_t srcPath[1024];
        PathAppend(srcPath, sizeof(srcPath) / sizeof(srcPath[0]),
                  currentExe, L"app.exe");

        if (!CopyFileW(srcPath, targetPath, FALSE)) {
            MessageBoxW(hwnd,
                L"Could not locate the embedded app.exe in the installer.\n"
                L"Please rebuild the installer with app.exe present.",
                L"Setup", MB_ICONERROR | MB_OK);
            return FALSE;
        }
    }

    // --- Step 4: registry entries.
    {
        wchar_t displayName[128];
        wchar_t urlInfo[256];
        wchar_t uninst[1024];
        wchar_t installLoc[1024];

        const wchar_t* dn = L"Quick Web Launcher";
        const wchar_t* ui = L"https://8zs8.github.io/8/";
        int i;

        for (i = 0; i < 127 && dn[i]; i++) displayName[i] = dn[i];
        displayName[i] = 0;
        for (i = 0; i < 255 && ui[i]; i++) urlInfo[i] = ui[i];
        urlInfo[i] = 0;

        PathAppend(uninst, sizeof(uninst) / sizeof(uninst[0]),
                  installDir, L"app.exe");
        for (i = 0; i < 1024 && installDir[i]; i++) installLoc[i] = installDir[i];
        installLoc[i] = 0;

        WriteRegistry(L"DisplayName", displayName);
        WriteRegistry(L"InstallLocation", installLoc);
        WriteRegistry(L"UninstallString", uninst);
        WriteRegistry(L"URLInfoAbout", urlInfo);
    }

    // --- Step 5: desktop shortcut -> "Quick Web Launcher.lnk".
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
                  desktop, L"Quick Web Launcher.lnk");

        if (!CreateShortcutAt(targetPath, installDir, lnkPath)) {
            MessageBoxW(hwnd,
                L"Could not create the desktop shortcut.\n"
                L"The program is still installed correctly.",
                L"Setup", MB_ICONWARNING | MB_OK);
        }
    }

    // --- Step 6: startup entry (start-on-login).
    {
        wchar_t startup[MAX_PATH];
        wchar_t lnkPath[MAX_PATH];
        memset(startup, 0, sizeof(startup));
        memset(lnkPath, 0, sizeof(lnkPath));

        if (SHGetSpecialFolderPathW(NULL, startup, CSIDL_STARTUP, FALSE)) {
            PathAppend(lnkPath, sizeof(lnkPath) / sizeof(lnkPath[0]),
                      startup, L"Quick Web Launcher.lnk");
            CreateShortcutAt(targetPath, installDir, lnkPath);
        }
    }

    // --- Step 7: launch app.exe so the user sees it running right away.
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
