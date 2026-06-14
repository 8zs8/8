#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shellapi.h>

/* ====================================================================
   链接库声明（两种方式同时生效，确保链接成功）
   方式 A：Dev-C++ 的 App.dev 中 Libs= 字段会把 -lgdi32 等传给链接器
   方式 B：以下 #pragma comment(lib,...) 在 .obj 中嵌入链接指示，
           链接器读到 .obj 时会自动加入对应库（双保险）
   ==================================================================== */
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "kernel32.lib")

/* ====================================================================
   宏定义与全局变量
   ==================================================================== */
#define ICON_RES_ID         MAKEINTRESOURCEW(1)
#define FLOAT_WIN_CLASS     L"QR_Float_001"
#define MSG_WIN_CLASS       L"QR_Msg_001"
#define FLOAT_SIZE          64
#define DRAG_THRESHOLD      5
#define LONGPRESS_MS        800
#define TIMER_LONGPRESS     1001

static HWND  g_hMsgWnd    = NULL;
static HWND  g_hFloatWnd  = NULL;
static HICON g_hIcon      = NULL;
static HICON g_hIconSm    = NULL;

static BOOL  g_isDown     = FALSE;
static BOOL  g_isDragging = FALSE;
static BOOL  g_longPressed= FALSE;
static int   g_dragDX     = 0;
static int   g_dragDY     = 0;
static int   g_downX      = 0;
static int   g_downY      = 0;

/* ====================================================================
   前向声明
   ==================================================================== */
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
static void  OpenTargetUrl(void);
static void  ShowFloatPopup(HWND hwnd);
static void  ShowTrayPopup(HWND hwnd);

/* ====================================================================
   WinMain —— 程序入口
   ==================================================================== */
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    WNDCLASSEXW wc;
    NOTIFYICONDATAW nid;
    int screenW, screenY, x, y;
    MSG msg;

    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    /* 从资源加载自定义图标（app.rc 中声明为 1 ICON "app.ico"） */
    g_hIcon   = LoadIconW(GetModuleHandleW(NULL), ICON_RES_ID);
    g_hIconSm = LoadIconW(GetModuleHandleW(NULL), ICON_RES_ID);

    /* 兜底：如果 app.ico 未被正确编译进资源，则使用系统默认图标 */
    if (!g_hIcon)   g_hIcon   = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
    if (!g_hIconSm) g_hIconSm = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);

    /* --- 注册消息窗口类（处理托盘事件） --- */
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = MsgWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = MSG_WIN_CLASS;
    wc.hIcon         = g_hIcon;    /* 任务栏/任务管理器显示的大图标 */
    wc.hIconSm       = g_hIconSm;  /* 任务栏/任务管理器显示的小图标 */
    RegisterClassExW(&wc);

    /* --- 注册悬浮窗口类（右下角置顶按钮） --- */
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = FloatWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = FLOAT_WIN_CLASS;
    wc.hIcon         = g_hIcon;
    wc.hIconSm       = g_hIconSm;
    RegisterClassExW(&wc);

    /* --- 创建消息窗口（隐藏，仅用于接收托盘消息） --- */
    g_hMsgWnd = CreateWindowExW(0, MSG_WIN_CLASS, L"", 0,
                                0, 0, 0, 0, HWND_MESSAGE,
                                NULL, hInstance, NULL);
    if (g_hMsgWnd) {
        /* 再次显式设置图标（双保险） */
        SendMessageW(g_hMsgWnd, WM_SETICON, ICON_BIG,   (LPARAM)g_hIcon);
        SendMessageW(g_hMsgWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSm);
    }

    /* --- 创建悬浮按钮（右下角，置顶，无边框） --- */
    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenY = GetSystemMetrics(SM_CYSCREEN);
    x = screenW - FLOAT_SIZE - 40;
    y = screenY - FLOAT_SIZE - 120;
    if (x < 0) x = 32;
    if (y < 0) y = 32;

    g_hFloatWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        FLOAT_WIN_CLASS, L"",
        WS_POPUP,
        x, y, FLOAT_SIZE, FLOAT_SIZE,
        NULL, NULL, hInstance, NULL);

    if (g_hFloatWnd) {
        SendMessageW(g_hFloatWnd, WM_SETICON, ICON_BIG,   (LPARAM)g_hIcon);
        SendMessageW(g_hFloatWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSm);
        ShowWindow(g_hFloatWnd, SW_SHOW);
        UpdateWindow(g_hFloatWnd);
        SetWindowPos(g_hFloatWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    /* --- 添加系统托盘图标（使用同一个图标资源） --- */
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd   = g_hMsgWnd;
    nid.uID    = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 100;
    nid.hIcon  = g_hIcon;   /* 托盘使用同一个 app.ico */
    nid.szTip[0] = L'\0';
    Shell_NotifyIconW(NIM_ADD, &nid);

    /* --- 消息循环 --- */
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    /* --- 清理 --- */
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd   = g_hMsgWnd;
    nid.uID    = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);

    if (g_hIcon)   DestroyIcon(g_hIcon);
    if (g_hIconSm) DestroyIcon(g_hIconSm);

    return (int)msg.wParam;
}

/* ====================================================================
   打开目标网页
   ==================================================================== */
static void OpenTargetUrl(void)
{
    ShellExecuteW(NULL, L"open",
                  L"https://8zs8.github.io/8/",
                  NULL, NULL, SW_SHOWNORMAL);
}

/* ====================================================================
   悬浮按钮右键菜单
   ==================================================================== */
static void ShowFloatPopup(HWND hwnd)
{
    POINT pt;
    HMENU menu;
    UINT clicked;

    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);

    menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, L"Open");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 1002, L"Exit");

    clicked = TrackPopupMenu(menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, hwnd, NULL);

    if (clicked == 1001)
        OpenTargetUrl();
    else if (clicked == 1002) {
        DestroyWindow(g_hFloatWnd);
        if (g_hMsgWnd) DestroyWindow(g_hMsgWnd);
    }
    DestroyMenu(menu);
}

/* ====================================================================
   托盘右键菜单
   ==================================================================== */
static void ShowTrayPopup(HWND hwnd)
{
    POINT pt;
    HMENU menu;
    UINT clicked;

    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);

    menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, L"Open");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 1002, L"Exit");

    clicked = TrackPopupMenu(menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, hwnd, NULL);

    if (clicked == 1001)
        OpenTargetUrl();
    else if (clicked == 1002) {
        DestroyWindow(g_hFloatWnd);
        DestroyWindow(hwnd);
    }
    DestroyMenu(menu);
}

/* ====================================================================
   消息窗口过程（处理托盘事件）
   ==================================================================== */
LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UINT mouseMsg;

    switch (msg)
    {
        case WM_USER + 100:
            mouseMsg = (UINT)lParam;
            if (mouseMsg == WM_LBUTTONDOWN || mouseMsg == WM_LBUTTONDBLCLK) {
                OpenTargetUrl();
                return 0;
            }
            if (mouseMsg == WM_RBUTTONUP) {
                ShowTrayPopup(hwnd);
                return 0;
            }
            return 0;

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

/* ====================================================================
   悬浮按钮窗口过程
   ==================================================================== */
LRESULT CALLBACK FloatWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        /* --- 绘制：浅色背景 + 居中显示 app.ico --- */
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT rc;

            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rc);

            /* 浅色背景 */
            FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));

            /* 居中绘制 app.ico（使用全局缓存的 g_hIcon） */
            if (g_hIcon) {
                DrawIconEx(hdc,
                    (FLOAT_SIZE - 32) / 2,
                    (FLOAT_SIZE - 32) / 2,
                    g_hIcon, 32, 32, 0, NULL, DI_NORMAL);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        /* --- 左键按下：记录位置，启动长按计时器 --- */
        case WM_LBUTTONDOWN:
        {
            POINT pt;
            RECT rc;

            g_isDown     = TRUE;
            g_isDragging = FALSE;
            g_longPressed= FALSE;

            SetCapture(hwnd);
            GetCursorPos(&pt);
            g_downX = pt.x;
            g_downY = pt.y;

            GetWindowRect(hwnd, &rc);
            g_dragDX = pt.x - rc.left;
            g_dragDY = pt.y - rc.top;

            SetTimer(hwnd, TIMER_LONGPRESS, LONGPRESS_MS, NULL);
            return 0;
        }

        /* --- 鼠标移动：超过阈值则开始拖动 --- */
        case WM_MOUSEMOVE:
        {
            if (g_isDown && !g_longPressed) {
                POINT pt;
                int dx, dy;

                GetCursorPos(&pt);
                dx = pt.x - g_downX;
                dy = pt.y - g_downY;

                if ((dx * dx + dy * dy) > (DRAG_THRESHOLD * DRAG_THRESHOLD)) {
                    if (!g_isDragging) {
                        g_isDragging = TRUE;
                        KillTimer(hwnd, TIMER_LONGPRESS);
                    }
                }

                if (g_isDragging) {
                    SetWindowPos(hwnd, HWND_TOPMOST,
                                 pt.x - g_dragDX, pt.y - g_dragDY,
                                 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
                }
            }
            return 0;
        }

        /* --- 左键抬起：未拖动且未长按则视为单击，打开网页 --- */
        case WM_LBUTTONUP:
        {
            KillTimer(hwnd, TIMER_LONGPRESS);
            if (g_isDown) {
                ReleaseCapture();
                if (!g_longPressed && !g_isDragging) {
                    OpenTargetUrl();
                }
            }
            g_isDown     = FALSE;
            g_isDragging = FALSE;
            g_longPressed= FALSE;
            return 0;
        }

        /* --- 右键：直接弹出菜单 --- */
        case WM_RBUTTONUP:
            ShowFloatPopup(hwnd);
            return 0;

        /* --- 长按计时器：超过 800ms 视为右键 --- */
        case WM_TIMER:
            if (wParam == TIMER_LONGPRESS && g_isDown) {
                g_longPressed = TRUE;
                KillTimer(hwnd, TIMER_LONGPRESS);
                if (!g_isDragging) {
                    ReleaseCapture();
                    g_isDown = FALSE;
                    ShowFloatPopup(hwnd);
                }
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            return 0;

        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
