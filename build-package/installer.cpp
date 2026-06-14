// ============================================================
//  抽人软件 - 安装程序
//  创建快捷方式、注册开机自启
//
//  编译：Dev-C++ 中打开 Installer.dev，按 Ctrl+F9
//============================================================

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

// ============================================================
//  消除控制台黑窗 (最可靠方式, 不依赖项目配置)
// ============================================================
#pragma comment(linker, "-mwindows")

// ============================================================
//  强制链接库
// ============================================================
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

// ============================================================
//  ID
// ============================================================
#define BTN_INSTALL 2001
#define BTN_CANCEL  2002
#define LBL_STATUS  2003

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static BOOL DoInstall(void);

// ============================================================
//  WinMain
// ============================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"IWnd";
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    RegisterClassExW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    HWND hWnd = CreateWindowExW(0, L"IWnd", L"抽人软件 - 安装",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        (sw-400)/2, (sh-240)/2, 400, 240,
        NULL, NULL, hInst, NULL);
    if (!hWnd) return 1;

    CreateWindowExW(0, L"BUTTON", L"快速安装",
        WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
        60, 130, 120, 36, hWnd,
        (HMENU)BTN_INSTALL, hInst, NULL);
    CreateWindowExW(0, L"BUTTON", L"退出",
        WS_VISIBLE|WS_CHILD,
        220, 130, 120, 36, hWnd,
        (HMENU)BTN_CANCEL, hInst, NULL);

    HWND hLbl = CreateWindowExW(0, L"STATIC",
        L"点击 快速安装 开始安装",
        WS_VISIBLE|WS_CHILD|SS_CENTER,
        50, 60, 300, 30, hWnd,
        (HMENU)LBL_STATUS, hInst, NULL);
    HFONT hF = CreateFontW(18,0,0,0,FW_NORMAL,0,0,0,
        DEFAULT_CHARSET,0,0,0,0,NULL);
    if (hF && hLbl) SendMessageW(hLbl, WM_SETFONT, (WPARAM)hF, TRUE);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return (int)msg.wParam;
}

// ============================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc; GetClientRect(hWnd, &rc);
        TRIVERTEX vt[2] = {
            {rc.left, rc.top, 0x40<<8,0x90<<8,0xE0<<8,0xFF<<8},
            {rc.right, rc.bottom, 0x10<<8,0x50<<8,0xC0<<8,0xFF<<8}
        };
        GRADIENT_RECT gr = {0,1};
        GradientFill(hdc, vt, 2, &gr, 1, GRADIENT_FILL_RECT_V);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_COMMAND:
    {
        if (LOWORD(w) == BTN_CANCEL) { DestroyWindow(hWnd); return 0; }
        if (LOWORD(w) == BTN_INSTALL)
        {
            HWND hS = GetDlgItem(hWnd, LBL_STATUS);
            if (hS) SetWindowTextW(hS, L"正在安装...");
            EnableWindow(GetDlgItem(hWnd, BTN_INSTALL), FALSE);

            if (DoInstall())
            {
                if (hS) SetWindowTextW(hS, L"安装成功！");
                MessageBoxW(hWnd,
                    L"抽人软件 安装完成！\n已创建桌面快捷方式\n已设置开机自启",
                    L"成功", MB_ICONINFORMATION|MB_OK);
                DestroyWindow(hWnd);
            }
            else
            {
                if (hS) SetWindowTextW(hS, L"安装失败");
                EnableWindow(GetDlgItem(hWnd, BTN_INSTALL), TRUE);
                MessageBoxW(hWnd, L"安装出错，请重试", L"错误", MB_ICONERROR|MB_OK);
            }
        }
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, w, l);
}

// ============================================================
static BOOL DoInstall(void)
{
    WCHAR dir[MAX_PATH], target[MAX_PATH];

    // %APPDATA%\ChouRen
    SHGetSpecialFolderPathW(NULL, dir, CSIDL_APPDATA, FALSE);
    lstrcatW(dir, L"\\ChouRen");
    CreateDirectoryW(dir, NULL);

    // 路径
    lstrcpyW(target, dir);
    lstrcatW(target, L"\\App.exe");

    // 如果 App.exe 存在则跳过解压
    if (GetFileAttributesW(target) == INVALID_FILE_ATTRIBUTES)
    {
        // 从资源中提取
        HINSTANCE h = GetModuleHandleW(NULL);
        HRSRC hr = FindResourceW(h, L"APP_EXE", L"BINARY");
        if (hr)
        {
            HGLOBAL hg = LoadResource(h, hr);
            if (hg)
            {
                DWORD sz = SizeofResource(h, hr);
                void* p = LockResource(hg);
                if (p)
                {
                    HANDLE hf = CreateFileW(target, GENERIC_WRITE,0,NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hf != INVALID_HANDLE_VALUE)
                    {
                        DWORD w;
                        WriteFile(hf, p, sz, &w, NULL);
                        CloseHandle(hf);
                    }
                }
            }
        }
    }

    // --- 桌面快捷方式 ---
    {
        WCHAR desktop[MAX_PATH];
        SHGetSpecialFolderPathW(NULL, desktop, CSIDL_DESKTOPDIRECTORY, FALSE);
        lstrcatW(desktop, L"\\抽人软件.lnk");

        IShellLinkW* psl;
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL,
            CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)))
        {
            psl->SetPath(target);
            psl->SetDescription(L"抽人软件");
            IPersistFile* ppf;
            if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)))
            {
                ppf->Save(desktop, TRUE);
                ppf->Release();
            }
            psl->Release();
        }
    }

    // --- 开机自启 (启动文件夹) ---
    {
        WCHAR startup[MAX_PATH];
        SHGetSpecialFolderPathW(NULL, startup, CSIDL_STARTUP, FALSE);
        lstrcatW(startup, L"\\抽人软件.lnk");

        IShellLinkW* psl;
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL,
            CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)))
        {
            psl->SetPath(target);
            psl->SetDescription(L"抽人软件");
            IPersistFile* ppf;
            if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)))
            {
                ppf->Save(startup, TRUE);
                ppf->Release();
            }
            psl->Release();
        }
    }

    return TRUE;
}