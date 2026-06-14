// ============================================================
//  抽人软件 - 主程序
//  右下角悬浮按钮，点击打开网页，可拖动，托盘图标
//
//  编译方法：Dev-C++ 中打开 App.dev，按 Ctrl+F9
//  如果链接报错，在 Project -> Project Options -> Parameters
//  Linker 框中粘贴:
//   -mwindows -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
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

// ============================================================
//  消除控制台黑窗 (方法1: 编译器从源代码中读取此指令)
//  原理: #pragma comment(linker, "xxx") 把链接器指令写入
//  目标文件(.o)，链接器读取后会将程序标记为 GUI 子系统，
//  Windows 就不会为此程序创建控制台窗口。
//  这种方式最可靠，不依赖 Dev-C++ 项目配置。
// ============================================================
#pragma comment(linker, "-mwindows")

// ============================================================
//  强制链接所需库 (方法2: 同样写入目标文件)
//  即使 Dev-C++ 工程文件的 Linker 参数失效也能链接成功
// ============================================================
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "comctl32")

// ============================================================
//  常量
// ============================================================
#define TRAY_ID         1001
#define MENU_OPEN       1002
#define MENU_EXIT       1003
#define TRAY_MSG        (WM_USER + 100)

#define FLOAT_W         68
#define FLOAT_H         68

// ============================================================
//  全局变量
// ============================================================
static HWND g_hWnd   = NULL;
static HWND g_hMsg   = NULL;
static BOOL  g_bDrag = FALSE;
static int   g_dx, g_dy;

// ============================================================
//  函数声明
// ============================================================
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MsgProc(HWND, UINT, WPARAM, LPARAM);
static void OpenUrl(void);
static void ShowMenu(HWND);

// ============================================================
//  WinMain - 入口
// ============================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    // --- 注册悬浮窗口类 ---
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FloatWnd";
    wc.hIcon         = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    RegisterClassExW(&wc);

    // --- 注册消息窗口类 ---
    WNDCLASSEXW mc = {0};
    mc.cbSize        = sizeof(mc);
    mc.lpfnWndProc   = MsgProc;
    mc.hInstance     = hInst;
    mc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    mc.lpszClassName = L"MsgWnd";
    RegisterClassExW(&mc);

    // --- 创建消息窗口 ---
    g_hMsg = CreateWindowExW(0, L"MsgWnd", NULL, 0, 0,0,0,0,
                             HWND_MESSAGE, NULL, hInst, NULL);

    // --- 创建悬浮窗口 (右下角) ---
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"FloatWnd", L"",
        WS_POPUP,
        sw - FLOAT_W - 40, sh - FLOAT_H - 80,
        FLOAT_W, FLOAT_H,
        NULL, NULL, hInst, NULL);
    if (!g_hWnd) return 1;

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    // --- 添加托盘图标 ---
    NOTIFYICONDATAW nd = {0};
    nd.cbSize = sizeof(nd);
    nd.hWnd   = g_hMsg;
    nd.uID    = TRAY_ID;
    nd.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nd.uCallbackMessage = TRAY_MSG;
    nd.hIcon  = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    if (!nd.hIcon) nd.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
    lstrcpynW(nd.szTip, L"抽人软件", 128);
    Shell_NotifyIconW(NIM_ADD, &nd);

    // --- 消息循环 ---
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // --- 退出清理 ---
    nd.cbSize = sizeof(nd);
    nd.hWnd   = g_hMsg;
    nd.uID    = TRAY_ID;
    Shell_NotifyIconW(NIM_DELETE, &nd);
    return (int)msg.wParam;
}

// ============================================================
//  打开网页
// ============================================================
static void OpenUrl(void)
{
    ShellExecuteW(NULL, L"open",
        L"https://8zs8.github.io/8/", NULL, NULL, SW_SHOWNORMAL);
}

// ============================================================
//  右键菜单
// ============================================================
static void ShowMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, MENU_OPEN, L"打开网页");
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, MENU_EXIT, L"退出");
    UINT c = TrackPopupMenu(m, TPM_RETURNCMD|TPM_RIGHTBUTTON,
                             pt.x, pt.y, 0, hwnd, NULL);
    if (c == MENU_OPEN) OpenUrl();
    else if (c == MENU_EXIT) PostQuitMessage(0);
    DestroyMenu(m);
}

// ============================================================
//  悬浮按钮窗口过程
// ============================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // 蓝色渐变背景
        TRIVERTEX vt[2] = {
            {rc.left, rc.top, 0x40<<8, 0x90<<8, 0xE0<<8, 0xFF<<8},
            {rc.right, rc.bottom, 0x10<<8, 0x50<<8, 0xC0<<8, 0xFF<<8}
        };
        GRADIENT_RECT gr = {0,1};
        GradientFill(hdc, vt, 2, &gr, 1, GRADIENT_FILL_RECT_V);
        
        // 图标
        HICON hIcon = LoadIconW((HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE),
                                MAKEINTRESOURCEW(1));
        if (!hIcon) hIcon = LoadIconW(NULL,(LPCWSTR)IDI_APPLICATION);
        DrawIconEx(hdc, 10, 10, hIcon, 48, 48, 0, NULL, DI_NORMAL);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        g_bDrag = TRUE;
        SetCapture(hWnd);
        POINT p; GetCursorPos(&p);
        RECT  r; GetWindowRect(hWnd, &r);
        g_dx = p.x - r.left;
        g_dy = p.y - r.top;
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (!g_bDrag) return 0;
        POINT p; GetCursorPos(&p);
        SetWindowPos(hWnd, HWND_TOPMOST,
                     p.x - g_dx, p.y - g_dy,
                     0,0, SWP_NOSIZE|SWP_NOACTIVATE);
        return 0;
    }
    case WM_LBUTTONUP:
    {
        if (!g_bDrag) return 0;
        ReleaseCapture();
        g_bDrag = FALSE;
        OpenUrl();
        return 0;
    }
    case WM_RBUTTONUP:
        ShowMenu(hWnd);
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hWnd, msg, w, l);
}

// ============================================================
//  消息窗口 - 托盘图标回调
// ============================================================
LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    if (msg == TRAY_MSG)
    {
        if (l == WM_LBUTTONDOWN || l == WM_LBUTTONDBLCLK)
            OpenUrl();
        else if (l == WM_RBUTTONUP)
            ShowMenu(hWnd);
        return 0;
    }
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProcW(hWnd, msg, w, l);
}