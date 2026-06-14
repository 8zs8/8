#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

#define ICON_RES_ID         1
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
static int   g_dragDX     = 0;
static int   g_dragDY     = 0;
static int   g_downX      = 0;
static int   g_downY      = 0;
static BOOL  g_longPressed= FALSE;

static const wchar_t g_targetUrl[] = L"https://8zs8.github.io/8/";
static const wchar_t g_appName[]   = L"Chou Ren";

LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
static void ShowFloatPopup(HWND hwnd);
static void ShowTrayPopup(HWND hwnd);

static void LoadAppIcons(HINSTANCE hInstance)
{
    HINSTANCE hMod = GetModuleHandleW(NULL);
    if (!hMod) hMod = hInstance;

    g_hIcon = (HICON)LoadImageW(hMod, MAKEINTRESOURCEW(ICON_RES_ID),
                                IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    if (!g_hIcon)
        g_hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);

    g_hIconSm = (HICON)LoadImageW(hMod, MAKEINTRESOURCEW(ICON_RES_ID),
                                  IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!g_hIconSm)
        g_hIconSm = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    LoadAppIcons(hInstance);

    /* --- 注册消息窗口类 --- */
    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc   = MsgWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = MSG_WIN_CLASS;
        wc.hIcon         = g_hIcon;
        wc.hIconSm       = g_hIconSm;
        RegisterClassExW(&wc);
    }

    /* --- 注册悬浮窗口类 --- */
    {
        WNDCLASSEXW wc;
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
    }

    /* --- 创建消息窗口（托盘事件） --- */
    g_hMsgWnd = CreateWindowExW(0, MSG_WIN_CLASS, L"", 0,
                                0, 0, 0, 0, HWND_MESSAGE,
                                NULL, hInstance, NULL);

    /* --- 创建悬浮按钮（置顶、无边框、64x64） --- */
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int x = screenW - FLOAT_SIZE - 40;
        int y = screenH - FLOAT_SIZE - 120;
        if (x < 0) x = 32;
        if (y < 0) y = 32;

        g_hFloatWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            FLOAT_WIN_CLASS, L"",
            WS_POPUP,
            x, y, FLOAT_SIZE, FLOAT_SIZE,
            NULL, NULL, hInstance, NULL);

        if (g_hFloatWnd) {
            ShowWindow(g_hFloatWnd, SW_SHOW);
            UpdateWindow(g_hFloatWnd);
            SetWindowPos(g_hFloatWnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    /* --- 添加系统托盘图标 --- */
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd   = g_hMsgWnd;
        nid.uID    = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER + 100;
        nid.hIcon  = g_hIcon;
        wcsncpy(nid.szTip, g_appName, 127);
        nid.szTip[127] = L'\0';
        Shell_NotifyIconW(NIM_ADD, &nid);
    }

    /* --- 消息循环 --- */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    /* --- 清理 --- */
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd   = g_hMsgWnd;
        nid.uID    = 1;
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }
    if (g_hIcon)   DestroyIcon(g_hIcon);
    if (g_hIconSm) DestroyIcon(g_hIconSm);

    return (int)msg.wParam;
}

static void OpenTargetUrl(void)
{
    ShellExecuteW(NULL, L"open", g_targetUrl, NULL, NULL, SW_SHOWNORMAL);
}

static void ShowFloatPopup(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, L"Open URL");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 1002, L"Exit");
    UINT clicked = TrackPopupMenu(menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, hwnd, NULL);
    if (clicked == 1001) OpenTargetUrl();
    else if (clicked == 1002) {
        DestroyWindow(g_hFloatWnd);
        if (g_hMsgWnd) DestroyWindow(g_hMsgWnd);
    }
    DestroyMenu(menu);
}

static void ShowTrayPopup(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, L"Open URL");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 1002, L"Exit");
    UINT clicked = TrackPopupMenu(menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, hwnd, NULL);
    if (clicked == 1001) OpenTargetUrl();
    else if (clicked == 1002) {
        DestroyWindow(g_hFloatWnd);
        DestroyWindow(hwnd);
    }
    DestroyMenu(menu);
}

LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_USER + 100:
        {
            UINT mouseMsg = (UINT)lParam;
            if (mouseMsg == WM_LBUTTONDOWN || mouseMsg == WM_LBUTTONDBLCLK) {
                OpenTargetUrl();
                return 0;
            }
            if (mouseMsg == WM_RBUTTONUP) {
                ShowTrayPopup(hwnd);
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

LRESULT CALLBACK FloatWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            /* --- 绘制浅色背景（纯色，不使用渐变，避免链接问题） --- */
            HBRUSH hBrush = CreateSolidBrush(RGB(210, 230, 255));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);

            /* --- 绘制图标（居中，32x32 或 48x48） --- */
            if (g_hIcon) {
                int iconSize = 48;
                int ix = (FLOAT_SIZE - iconSize) / 2;
                int iy = (FLOAT_SIZE - iconSize) / 2;
                DrawIconEx(hdc, ix, iy, g_hIcon, iconSize, iconSize,
                           0, NULL, DI_NORMAL);
            }

            /* --- 绘制细边框（可选） --- */
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(120, 150, 200));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            MoveToEx(hdc, 0, 0, NULL);
            LineTo(hdc, FLOAT_SIZE - 1, 0);
            LineTo(hdc, FLOAT_SIZE - 1, FLOAT_SIZE - 1);
            LineTo(hdc, 0, FLOAT_SIZE - 1);
            LineTo(hdc, 0, 0);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            g_isDown     = TRUE;
            g_isDragging = FALSE;
            g_longPressed= FALSE;
            SetCapture(hwnd);
            POINT pt;
            GetCursorPos(&pt);
            g_downX = pt.x;
            g_downY = pt.y;
            RECT rc;
            GetWindowRect(hwnd, &rc);
            g_dragDX = pt.x - rc.left;
            g_dragDY = pt.y - rc.top;
            SetTimer(hwnd, TIMER_LONGPRESS, LONGPRESS_MS, NULL);
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (g_isDown && !g_longPressed) {
                POINT pt;
                GetCursorPos(&pt);
                int dx = pt.x - g_downX;
                int dy = pt.y - g_downY;
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

        case WM_RBUTTONUP:
        {
            ShowFloatPopup(hwnd);
            return 0;
        }

        case WM_TIMER:
        {
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
