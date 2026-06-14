// ====================================================================
//  抽人软件 - 主程序
//  功能：右下角悬浮置顶小图标 + 系统托盘图标
//        单击悬浮图标 -> 打开网页
//        拖动悬浮图标 -> 移动位置（不打开网页）
//        长按悬浮图标 -> 弹出右键菜单
//  编译：在 Dev-C++ 中打开 App.dev，按 Ctrl+F9
// ====================================================================

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>

// --- 链接器指令：确保无控制台窗口 ---
#pragma comment(linker, "-mwindows")

// --- 依赖库 ---
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

// --- 前向声明 ---
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
static void OpenTargetUrl(void);
static void ShowFloatPopupMenu(HWND hwnd);
static void ShowTrayPopupMenu(HWND hwnd);

#define ICON_RES_ID         1
#define FLOAT_WIN_CLASS     L"QRFloatClass"
#define MSG_WIN_CLASS       L"QRMsgClass"

#define FLOAT_SIZE          64       // 悬浮图标大小（像素）
#define DRAG_THRESHOLD      5        // 拖动阈值（像素），超过则判定为拖动
#define LONGPRESS_MS        800      // 长按判定时间（毫秒）

#define TIMER_LONGPRESS     1001

static HWND g_hMsgWnd    = NULL;
static HWND g_hFloatWnd  = NULL;

// 拖动状态
static BOOL g_isDown     = FALSE;   // 鼠标按下
static BOOL g_isDragging = FALSE;   // 已判定为拖动
static int  g_dragDX     = 0;
static int  g_dragDY     = 0;
static int  g_downX      = 0;       // 按下时的屏幕坐标
static int  g_downY      = 0;
static DWORD g_downTime  = 0;       // 按下时的系统时间
static BOOL g_longPressed = FALSE;  // 是否触发了长按

static const wchar_t g_targetUrl[]  = L"https://8zs8.github.io/8/";
static const wchar_t g_appName[]    = L"抽人软件";

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

    // --- 注册消息窗口类 ---
    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc   = MsgWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = MSG_WIN_CLASS;
        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) return 1;
        }
    }

    // --- 注册悬浮窗口类（含透明背景） ---
    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc   = FloatWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);  // 透明背景
        wc.lpszClassName = FLOAT_WIN_CLASS;
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) return 1;
        }
    }

    // --- 创建消息窗口（托盘事件用，不可见） ---
    g_hMsgWnd = CreateWindowExW(
        0, MSG_WIN_CLASS, L"", 0,
        0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!g_hMsgWnd) return 1;

    // --- 创建悬浮置顶图标窗口 ---
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int x = screenW - FLOAT_SIZE - 40;
        int y = screenH - FLOAT_SIZE - 120;
        if (x < 0) x = 32;
        if (y < 0) y = 32;

        g_hFloatWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
            FLOAT_WIN_CLASS,
            L"",
            WS_POPUP,
            x, y, FLOAT_SIZE, FLOAT_SIZE,
            NULL, NULL, hInstance, NULL);

        if (!g_hFloatWnd) return 1;

        // 透明颜色键：洋红（RGB(255,0,255)）作为透明色
        SetLayeredWindowAttributes(g_hFloatWnd, RGB(255, 0, 255), 0, LWA_COLORKEY);

        ShowWindow(g_hFloatWnd, SW_SHOW);
        UpdateWindow(g_hFloatWnd);
        SetWindowPos(g_hFloatWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    // --- 添加系统托盘图标 ---
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd   = g_hMsgWnd;
        nid.uID    = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER + 100;
        nid.hIcon  = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RES_ID));
        if (!nid.hIcon)
            nid.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);

        // 设置 tooltip（程序名）
        int i;
        for (i = 0; i < 127 && g_appName[i]; i++) nid.szTip[i] = g_appName[i];
        nid.szTip[i] = 0;

        Shell_NotifyIconW(NIM_ADD, &nid);
    }

    // --- 消息循环 ---
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // --- 清理 ---
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd   = g_hMsgWnd;
        nid.uID    = 1;
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }

    return (int)msg.wParam;
}

// ====================================================================
//  打开目标网页
// ====================================================================
static void OpenTargetUrl(void)
{
    ShellExecuteW(NULL, L"open", g_targetUrl, NULL, NULL, SW_SHOWNORMAL);
}

// ====================================================================
//  弹出悬浮图标的右键菜单
// ====================================================================
static void ShowFloatPopupMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, L"打开网页");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 1002, L"退出");
    UINT clicked = TrackPopupMenu(
        menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, hwnd, NULL);
    if (clicked == 1001) OpenTargetUrl();
    else if (clicked == 1002) {
        DestroyWindow(g_hFloatWnd);
        if (g_hMsgWnd) DestroyWindow(g_hMsgWnd);
    }
    DestroyMenu(menu);
}

// ====================================================================
//  弹出托盘图标的菜单
// ====================================================================
static void ShowTrayPopupMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, L"打开网页");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 1002, L"退出");
    UINT clicked = TrackPopupMenu(
        menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, hwnd, NULL);
    if (clicked == 1001) OpenTargetUrl();
    else if (clicked == 1002) {
        DestroyWindow(g_hFloatWnd);
        DestroyWindow(hwnd);
    }
    DestroyMenu(menu);
}

// ====================================================================
//  消息窗口过程（处理托盘事件）
// ====================================================================
LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_USER + 100:
        {
            UINT mouseMsg = (UINT)lParam;
            if (mouseMsg == WM_LBUTTONDOWN || mouseMsg == WM_LBUTTONDBLCLK)
            {
                OpenTargetUrl();
                return 0;
            }
            if (mouseMsg == WM_RBUTTONUP)
            {
                ShowTrayPopupMenu(hwnd);
                return 0;
            }
            return 0;
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
//  悬浮图标窗口过程（处理单击 / 拖动 / 长按）
// ====================================================================
LRESULT CALLBACK FloatWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // 用洋红色（透明键颜色）填充整个窗口背景
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH magentaBrush = CreateSolidBrush(RGB(255, 0, 255));
            FillRect(hdc, &rc, magentaBrush);
            DeleteObject(magentaBrush);

            // 在中心绘制图标
            HICON hIcon = LoadIconW(
                (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
                MAKEINTRESOURCEW(ICON_RES_ID));
            if (!hIcon) hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
            if (hIcon) {
                int iconSize = FLOAT_SIZE - 4;  // 留 2px 边距
                DrawIconEx(hdc, 2, 2, hIcon, iconSize, iconSize,
                           0, NULL, DI_NORMAL);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            g_isDown     = TRUE;
            g_isDragging = FALSE;
            g_longPressed = FALSE;
            g_downTime   = GetTickCount();

            SetCapture(hwnd);
            POINT pt;
            GetCursorPos(&pt);
            g_downX = pt.x;
            g_downY = pt.y;

            RECT rc;
            GetWindowRect(hwnd, &rc);
            g_dragDX = pt.x - rc.left;
            g_dragDY = pt.y - rc.top;

            // 设置长按定时器
            SetTimer(hwnd, TIMER_LONGPRESS, LONGPRESS_MS, NULL);
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (g_isDown && !g_longPressed)
            {
                POINT pt;
                GetCursorPos(&pt);
                int dx = pt.x - g_downX;
                int dy = pt.y - g_downY;
                if ((dx * dx + dy * dy) > (DRAG_THRESHOLD * DRAG_THRESHOLD))
                {
                    // 移动距离超过阈值，判定为拖动
                    if (!g_isDragging)
                    {
                        g_isDragging = TRUE;
                        // 拖动开始，取消长按
                        KillTimer(hwnd, TIMER_LONGPRESS);
                    }
                }

                if (g_isDragging)
                {
                    SetWindowPos(hwnd, HWND_TOPMOST,
                                 pt.x - g_dragDX, pt.y - g_dragDY,
                                 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
                }
            }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            KillTimer(hwnd, TIMER_LONGPRESS);

            if (g_isDown)
            {
                ReleaseCapture();

                if (g_longPressed)
                {
                    // 长按已触发（右键菜单已弹出），不做额外动作
                }
                else if (g_isDragging)
                {
                    // 拖动结束，不打开网页
                }
                else
                {
                    // 纯单击 -> 打开网页
                    OpenTargetUrl();
                }
            }

            g_isDown     = FALSE;
            g_isDragging = FALSE;
            g_longPressed = FALSE;
            return 0;
        }

        case WM_RBUTTONUP:
        {
            ShowFloatPopupMenu(hwnd);
            return 0;
        }

        case WM_TIMER:
        {
            if (wParam == TIMER_LONGPRESS && g_isDown)
            {
                // 长按触发
                g_longPressed = TRUE;
                KillTimer(hwnd, TIMER_LONGPRESS);

                // 模拟右键：弹出菜单
                if (!g_isDragging)
                {
                    ReleaseCapture();
                    g_isDown = FALSE;
                    ShowFloatPopupMenu(hwnd);
                }
            }
            return 0;
        }

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
