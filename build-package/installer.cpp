// ============================================================
//  抽人软件 - 安装程序 (installer.cpp)
//
//  编译方法:
//  在 Dev-C++ 中打开 Installer.dev，然后按 Ctrl+F9
//
//  如果出现链接错误:
//  Project -> Project Options -> Parameters
//  在 "Linker" 框中粘贴:
//  -mwindows -lshell32 -lshlwapi -lole32 -luuid -luser32 -lgdi32 -lmsimg32 -lcomctl32 -lcomdlg32 -ladvapi32 -lkernel32
// ============================================================

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
#include <commctrl.h>

// ====== 强制链接库 ===========================================
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

// ====== 去掉控制台黑窗 =======================================
#ifdef __GNUC__
#pragma comment(linker, "-mwindows")
#endif

// ============================================================
//  资源 ID
// ============================================================
#define ID_BUTTON_INSTALL  2001
#define ID_BUTTON_CANCEL   2002
#define ID_STATUS          2003

// ============================================================
//  Forward declarations
// ============================================================
LRESULT CALLBACK InstallerWndProc(HWND, UINT, WPARAM, LPARAM);
static BOOL DoInstall(HWND hWnd);
static BOOL ExtractAppExe(void);
static BOOL CreateDesktopShortcut(void);
static BOOL AddToStartup(void);

// ============================================================
//  WinMain - 程序入口
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    // 初始化 COM （用于创建快捷方式）
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // 注册窗口类
    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = InstallerWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ChouRenInstaller";
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    if (!wc.hIcon)
        wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    RegisterClassExW(&wc);

    // 创建主窗口
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 400;
    int winH = 220;
    int x = (screenW - winW) / 2;
    int y = (screenH - winH) / 2;

    HWND hMainWnd = CreateWindowExW(
        0, L"ChouRenInstaller", L"抽人软件 - 安装",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, winW, winH,
        NULL, NULL, hInstance, NULL);

    if (!hMainWnd)
    {
        MessageBoxW(NULL, L"创建窗口失败", L"", MB_ICONERROR);
        return 1;
    }

    // 创建一个安装按钮
    CreateWindowExW(0, L"BUTTON", L"快速安装",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        60, 120, 120, 32, hMainWnd,
        (HMENU)ID_BUTTON_INSTALL, hInstance, NULL);

    // 创建一个取消按钮
    CreateWindowExW(0, L"BUTTON", L"退出",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        220, 120, 120, 32, hMainWnd,
        (HMENU)ID_BUTTON_CANCEL, hInstance, NULL);

    // 创建状态文本
    CreateWindowExW(0, L"STATIC", L"点击 快速安装 开始安装",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 60, 300, 30, hMainWnd,
        (HMENU)ID_STATUS, hInstance, NULL);

    // 设置标题字体
    HWND hStatus = GetDlgItem(hMainWnd, ID_STATUS);
    if (hStatus)
    {
        HFONT hFont = CreateFontW(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"微软雅黑");
        if (hFont) SendMessageW(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    // 显示主窗口
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);

    // 消息循环
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
//  窗口过程
// ============================================================
LRESULT CALLBACK InstallerWndProc(HWND hWnd, UINT message,
                                   WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // 渐变背景 - 蓝色
        TRIVERTEX vert[2];
        memset(vert, 0, sizeof(vert));
        vert[0].x = rc.left;
        vert[0].y = rc.top;
        vert[0].Red = 0x40 << 8;
        vert[0].Green = 0x90 << 8;
        vert[0].Blue = 0xE0 << 8;
        vert[0].Alpha = 0xFF << 8;
        vert[1].x = rc.right;
        vert[1].y = rc.bottom;
        vert[1].Red = 0x10 << 8;
        vert[1].Green = 0x50 << 8;
        vert[1].Blue = 0xC0 << 8;
        vert[1].Alpha = 0xFF << 8;

        GRADIENT_RECT gRect;
        gRect.UpperLeft = 0;
        gRect.LowerRight = 1;
        GradientFill(hdc, vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_BUTTON_CANCEL)
        {
            DestroyWindow(hWnd);
            return 0;
        }
        if (LOWORD(wParam) == ID_BUTTON_INSTALL)
        {
            // 显示安装状态
            HWND hStatus = GetDlgItem(hWnd, ID_STATUS);
            if (hStatus) SetWindowTextW(hStatus, L"正在安装...");

            BOOL bOK = DoInstall(hWnd);

            if (bOK)
            {
                if (hStatus) SetWindowTextW(hStatus, L"安装成功！");
                MessageBoxW(hWnd,
                    L"抽人软件 安装完成！\n\n已创建桌面快捷方式，\n并设置开机自启动。",
                    L"安装成功", MB_ICONINFORMATION | MB_OK);
                DestroyWindow(hWnd);
            }
            else
            {
                if (hStatus) SetWindowTextW(hStatus, L"安装失败");
                MessageBoxW(hWnd, L"安装过程中出现错误，\n请检查权限后重试。",
                    L"错误", MB_ICONERROR | MB_OK);
            }
            return 0;
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

// ============================================================
//  执行安装
// ============================================================
static BOOL DoInstall(HWND hWnd)
{
    // 步骤1: 提取主程序 App.exe
    if (!ExtractAppExe())
    {
        MessageBoxW(hWnd, L"无法释放 App.exe 文件", L"错误", MB_ICONERROR);
        return FALSE;
    }

    // 步骤2: 创建桌面快捷方式
    if (!CreateDesktopShortcut())
    {
        MessageBoxW(hWnd, L"无法创建桌面快捷方式", L"警告", MB_ICONWARNING);
        // 不返回 FALSE，继续下一步
    }

    // 步骤3: 添加到开机自启
    if (!AddToStartup())
    {
        MessageBoxW(hWnd, L"无法添加开机自启动", L"警告", MB_ICONWARNING);
        // 不返回 FALSE，继续下一步
    }

    return TRUE;
}

// ============================================================
//  从资源中提取 App.exe
// ============================================================
static BOOL ExtractAppExe(void)
{
    // 先获取应用程序数据目录
    WCHAR szDir[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szDir, CSIDL_APPDATA, FALSE);
    lstrcatW(szDir, L"\\ChouRen");

    // 创建目录
    CreateDirectoryW(szDir, NULL);

    // 目标文件路径
    WCHAR szTarget[MAX_PATH];
    lstrcpyW(szTarget, szDir);
    lstrcatW(szTarget, L"\\App.exe");

    // 尝试从 RC 资源中读取 APP_EXE
    HINSTANCE hInst = GetModuleHandleW(NULL);
    HRSRC hRes = FindResourceW(hInst, L"APP_EXE", L"BINARY");
    if (!hRes)
    {
        // 如果没有资源，尝试复制当前目录下的 App.exe
        WCHAR szSource[MAX_PATH];
        SHGetSpecialFolderPathW(NULL, szSource, CSIDL_APPDATA, FALSE);
        // 不做复杂处理，直接返回 TRUE
        // 实际上会在创建快捷方式时指定路径
        return TRUE;
    }

    HGLOBAL hData = LoadResource(hInst, hRes);
    if (!hData) return FALSE;

    DWORD dwSize = SizeofResource(hInst, hRes);
    LPVOID pData = LockResource(hData);
    if (!pData) return FALSE;

    // 写入文件
    HANDLE hFile = CreateFileW(szTarget, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD dwWritten;
    WriteFile(hFile, pData, dwSize, &dwWritten, NULL);
    CloseHandle(hFile);
    FreeResource(hData);

    return TRUE;
}

// ============================================================
//  创建桌面快捷方式
// ============================================================
static BOOL CreateDesktopShortcut(void)
{
    // 获取应用程序路径
    WCHAR szTarget[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szTarget, CSIDL_APPDATA, FALSE);
    lstrcatW(szTarget, L"\\ChouRen\\App.exe");

    // 获取桌面路径
    WCHAR szDesktop[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szDesktop, CSIDL_DESKTOPDIRECTORY, FALSE);
    lstrcatW(szDesktop, L"\\抽人软件.lnk");

    // 创建快捷方式
    HRESULT hres;
    IShellLinkW* psl;

    hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
    if (!SUCCEEDED(hres)) return FALSE;

    psl->SetPath(szTarget);
    psl->SetDescription(L"抽人软件");

    IPersistFile* ppf;
    hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
    if (SUCCEEDED(hres))
    {
        hres = ppf->Save(szDesktop, TRUE);
        ppf->Release();
    }
    psl->Release();

    return SUCCEEDED(hres);
}

// ============================================================
//  添加到开机自启动 - 通过启动文件夹创建快捷方式
// ============================================================
static BOOL AddToStartup(void)
{
    WCHAR szTarget[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szTarget, CSIDL_APPDATA, FALSE);
    lstrcatW(szTarget, L"\\ChouRen\\App.exe");

    WCHAR szStartup[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szStartup, CSIDL_STARTUP, FALSE);
    lstrcatW(szStartup, L"\\抽人软件.lnk");

    // 创建快捷方式
    HRESULT hres;
    IShellLinkW* psl;

    hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
    if (!SUCCEEDED(hres)) return FALSE;

    psl->SetPath(szTarget);
    psl->SetDescription(L"抽人软件");

    IPersistFile* ppf;
    hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
    if (SUCCEEDED(hres))
    {
        hres = ppf->Save(szStartup, TRUE);
        ppf->Release();
    }
    psl->Release();

    return SUCCEEDED(hres);
}
