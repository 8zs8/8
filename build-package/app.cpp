// ============================================================
//  抽人软件 - 主程序 (app.cpp)
//
//  编译方法:
//  在 Dev-C++ 中打开 App.dev，然后按 Ctrl+F9
//
//  如果出现链接错误:
//  Project -> Project Options -> Parameters
//  在 "Linker" 框中粘贴:
//  -mwindows -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
//
//  去掉黑窗的关键就是 -mwindows 这个链接器参数
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

// ====== 强制链接库 - 确保即使 Dev-C++ 项目配置错了也能链接 ====
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "comctl32")

// ============================================================
//  去掉控制台黑窗
//  说明: -mwindows 告诉链接器生成 GUI 子系统的程序
//  Windows 加载器看到 GUI 程序时就不会创建控制台窗口
// ============================================================
#ifdef __GNUC__
#pragma comment(linker, "-mwindows")
#endif

// ============================================================
//  常量定义
// ============================================================
#define ID_TRAY       1001
#define ID_OPEN_URL   1002
#define ID_EXIT       1003
#define FLOAT_WIDTH   64
#define FLOAT_HEIGHT  64
#define FLOAT_CLASS   L"ChouRenFloat"
#define MSG_TRAY     WM_USER + 200

// ============================================================
//  全局变量
// ============================================================
static HWND g_hFloatWnd = NULL;
static HWND g_hMsgWnd   = NULL;
static BOOL  g_bDragging   = FALSE;
static int   g_nDragX      = 0;
static int   g_nDragY      = 0;

// ============================================================
//  Forward declarations
// ============================================================
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
static void OpenURL(void);

// ============================================================
//  WinMain - 程序入口
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    // 注册悬浮按钮窗口类
    WNDCLASSEXW wcFloat;
    memset(&wcFloat, 0, sizeof(wcFloat));
    wcFloat.cbSize        = sizeof(WNDCLASSEXW);
    wcFloat.style         = CS_HREDRAW | CS_VREDRAW;
    wcFloat.lpfnWndProc   = FloatWndProc;
    wcFloat.hInstance     = hInstance;
    wcFloat.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
    wcFloat.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcFloat.lpszClassName = FLOAT_CLASS;
    wcFloat.hIcon       = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    if (!wcFloat.hIcon)
        wcFloat.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    RegisterClassExW(&wcFloat);

    // 注册消息窗口类（用于托盘消息）
    WNDCLASSEXW wcMsg;
    memset(&wcMsg, 0, sizeof(wcMsg));
    wcMsg.cbSize        = sizeof(WNDCLASSEXW);
    wcMsg.lpfnWndProc   = MsgWndProc;
    wcMsg.hInstance     = hInstance;
    wcMsg.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcMsg.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcMsg.lpszClassName = L"ChouRenMsg";
    RegisterClassExW(&wcMsg);

    // 创建消息窗口（用于托盘事件）
    g_hMsgWnd = CreateWindowExW(
        0, L"ChouRenMsg", L"", 0,
        0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL);

    // 获取屏幕大小，创建悬浮按钮在右下角
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = screenW - FLOAT_WIDTH - 40;
    int y = screenH - FLOAT_HEIGHT - 80;

    g_hFloatWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        FLOAT_CLASS,
        L"",
        WS_POPUP,
        x, y, FLOAT_WIDTH, FLOAT_HEIGHT,
        NULL, NULL, hInstance, NULL);

    if (!g_hFloatWnd)
    {
        MessageBoxW(NULL, L"创建窗口失败", L"", MB_ICONERROR);
        return 1;
    }

    // 让悬浮按钮窗口显示
    ShowWindow(g_hFloatWnd, SW_SHOW);
    UpdateWindow(g_hFloatWnd);

    // 添加托盘图标
    NOTIFYICONDATAW nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_hMsgWnd;
    nid.uID = ID_TRAY;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = MSG_TRAY;
    nid.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    if (!nid.hIcon)
        nid.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
    lstrcpynW(nid.szTip, L"抽人软件", 128);
    Shell_NotifyIconW(NIM_ADD, &nid);

    // 消息循环
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 清理
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_hMsgWnd;
    nid.uID = ID_TRAY;
    Shell_NotifyIconW(NIM_DELETE, &nid);

    return (int)msg.wParam;
}

// ============================================================
//  打开网页
// ============================================================
static void OpenURL(void)
{
    ShellExecuteW(NULL, L"open",
        L"https://8zs8.github.io/8/",
        NULL, NULL, SW_SHOWNORMAL);
}

// ============================================================
//  显示右键菜单
// ============================================================
static void ShowPopupMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_OPEN_URL, L"打开网页");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_EXIT, L"退出");

    UINT uClicked = TrackPopupMenu(
        hMenu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, hwnd, NULL);

    if (uClicked == ID_OPEN_URL)
        OpenURL();
    else if (uClicked == ID_EXIT)
    {
        DestroyWindow(g_hFloatWnd);
        PostQuitMessage(0);
    }
    DestroyMenu(hMenu);
}

// ============================================================
//  悬浮按钮窗口过程
// ============================================================
LRESULT CALLBACK FloatWndProc(HWND hWnd, UINT message,
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

        // 画图标
        HICON hIcon = LoadIconW(
            (HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE),
            MAKEINTRESOURCEW(1));
        if (!hIcon) hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
        if (hIcon)
        {
            DrawIconEx(hdc, 8, 8, hIcon, 48, 48, 0, NULL, DI_NORMAL);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        // 记录拖动
        g_bDragging = TRUE;
        SetCapture(hWnd);
        POINT pt;
        GetCursorPos(&pt);
        RECT rc;
        GetWindowRect(hWnd, &rc);
        g_nDragX = pt.x - rc.left;
        g_nDragY = pt.y - rc.top;
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (g_bDragging)
        {
            POINT pt;
            GetCursorPos(&pt);
            SetWindowPos(hWnd, HWND_TOPMOST,
                pt.x - g_nDragX, pt.y - g_nDragY,
                0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        if (g_bDragging)
        {
            ReleaseCapture();
            g_bDragging = FALSE;
            // 松开鼠标后打开网页
            OpenURL();
        }
        return 0;
    }

    case WM_RBUTTONUP:
    {
        ShowPopupMenu(hWnd);
        return 0;
    }

    case WM_DESTROY:
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

// ============================================================
//  消息窗口过程 - 处理托盘图标消息
// ============================================================
LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case MSG_TRAY:
    {
        if (LOWORD(lParam) == WM_LBUTTONDOWN ||
            LOWORD(lParam) == WM_LBUTTONDBLCLK)
        {
            OpenURL();
            return 0;
        }
        if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            ShowPopupMenu(hWnd);
            return 0;
        }
        return 0;
    }

    case WM_DESTROY:
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}
