#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_TRAY_ICON        1001
#define IDM_OPEN_URL        2001
#define IDM_EXIT            2002
#define WM_TRAYMESSAGE      (WM_USER + 1)
#define ICON_RESOURCE_ID    1
#define FLOAT_CLASS_NAME    L"FloatTopWindowClass"
#define HIDDEN_CLASS_NAME   L"HiddenTrayWindowClass"

static HINSTANCE g_hInstance = nullptr;
static HWND      g_hHidden  = nullptr;
static HWND      g_hFloat   = nullptr;
static NOTIFYICONDATA g_nid = {};

LRESULT CALLBACK HiddenWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CreateHiddenWindow();
BOOL    CreateFloatButton();
BOOL    ShowTrayIcon();
void    RemoveTrayIcon();
void    OpenWebPage();

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE /*hPrevInstance*/,
                   _In_ LPSTR     /*lpCmdLine*/,
                   _In_ int       /*nShowCmd*/)
{
    g_hInstance = hInstance;

    {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = HiddenWndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = HIDDEN_CLASS_NAME;
        wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);
    }

    {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = FloatWndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = FLOAT_CLASS_NAME;
        wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        wc.hCursor       = LoadCursorW(nullptr, IDC_HAND);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExW(&wc);
    }

    if (!CreateHiddenWindow())
    {
        MessageBoxW(nullptr, L"创建隐藏窗口失败", L"错误", MB_ICONERROR);
        return 1;
    }

    if (!CreateFloatButton())
    {
        MessageBoxW(nullptr, L"创建置顶按钮失败", L"错误", MB_ICONERROR);
        DestroyWindow(g_hHidden);
        return 1;
    }

    if (!ShowTrayIcon())
    {
        MessageBoxW(nullptr, L"创建托盘图标失败", L"错误", MB_ICONERROR);
        DestroyWindow(g_hFloat);
        DestroyWindow(g_hHidden);
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    RemoveTrayIcon();
    return (int)msg.wParam;
}

BOOL CreateHiddenWindow()
{
    g_hHidden = CreateWindowExW(
        0,
        HIDDEN_CLASS_NAME,
        L"TrayLauncher",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        g_hInstance,
        nullptr);

    if (!g_hHidden) return FALSE;

    SendMessageW(g_hHidden, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));
    SendMessageW(g_hHidden, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));

    return TRUE;
}

BOOL CreateFloatButton()
{
    int w = 64, h = 64;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = screenW - w - 20;
    int y = screenH - h - 80;

    g_hFloat = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        FLOAT_CLASS_NAME,
        L"",
        WS_POPUP,
        x, y, w, h,
        nullptr,
        nullptr,
        g_hInstance,
        nullptr);

    if (!g_hFloat) return FALSE;

    // 让窗口实际形状为圆形（圆角），避免出现白色方块
    HRGN hRgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, 20, 20);
    SetWindowRgn(g_hFloat, hRgn, TRUE);

    ShowWindow(g_hFloat, SW_SHOW);
    UpdateWindow(g_hFloat);
    return TRUE;
}

BOOL ShowTrayIcon()
{
    g_nid.cbSize           = sizeof(NOTIFYICONDATA);
    g_nid.hWnd             = g_hHidden;
    g_nid.uID              = ID_TRAY_ICON;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYMESSAGE;
    g_nid.hIcon            = LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    if (!g_nid.hIcon) g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    StringCchCopyW(g_nid.szTip, ARRAYSIZE(g_nid.szTip), L"点击打开网页");

    return Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon()
{
    if (g_nid.hWnd) Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void OpenWebPage()
{
    const wchar_t URL[] = L"https://8zs8.github.io/8/";

    INT_PTR res = (INT_PTR)ShellExecuteW(nullptr, L"open", URL,
                                         nullptr, nullptr, SW_SHOWNORMAL);
    if (res <= 32)
    {
        MessageBoxW(nullptr,
                    L"无法打开网页，请检查系统默认浏览器设置。",
                    L"提示", MB_ICONWARNING);
    }
}

LRESULT CALLBACK HiddenWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_TRAYMESSAGE:
        {
            UINT mouseMsg = (UINT)lParam;
            if (mouseMsg == WM_LBUTTONDOWN || mouseMsg == WM_LBUTTONDBLCLK)
            {
                OpenWebPage();
                return 0;
            }
            if (mouseMsg == WM_RBUTTONUP)
            {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, IDM_OPEN_URL, L"打开网页");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hMenu, MF_STRING, IDM_EXIT,     L"退出");
                SetForegroundWindow(hWnd);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON,
                               pt.x, pt.y, 0, hWnd, nullptr);
                DestroyMenu(hMenu);
                return 0;
            }
            return 0;
        }

        case WM_COMMAND:
        {
            WORD id = LOWORD(wParam);
            if (id == IDM_OPEN_URL) OpenWebPage();
            else if (id == IDM_EXIT)
            {
                if (g_hFloat) DestroyWindow(g_hFloat);
                DestroyWindow(hWnd);
            }
            return 0;
        }

        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK FloatWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool  s_dragging  = false;
    static bool  s_moved     = false;
    static POINT s_dragOffset = {};
    static POINT s_downPos    = {};

    switch (message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);

            // 渐变背景：蓝色系
            TRIVERTEX vert[2];
            vert[0].x     = 0;
            vert[0].y     = 0;
            vert[0].Red   = 0x40 << 8;
            vert[0].Green = 0x80 << 8;
            vert[0].Blue  = 0xE0 << 8;
            vert[0].Alpha = 0xFF << 8;

            vert[1].x     = rc.right;
            vert[1].y     = rc.bottom;
            vert[1].Red   = 0x70 << 8;
            vert[1].Green = 0xB0 << 8;
            vert[1].Blue  = 0xFF << 8;
            vert[1].Alpha = 0xFF << 8;

            GRADIENT_RECT grect;
            grect.UpperLeft  = 0;
            grect.LowerRight = 1;

            GradientFill(hdc, vert, 2, &grect, 1, GRADIENT_FILL_RECT_V);

            // 边框
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(40, 80, 200));
            HPEN hOld = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, 0, 0, rc.right, rc.bottom);
            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOld);
            DeleteObject(hPen);

            // 居中绘制图标
            HICON hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
            if (hIcon)
            {
                int iconSize = 32;
                int ix = (rc.right  - iconSize) / 2;
                int iy = (rc.bottom - iconSize) / 2;
                DrawIconEx(hdc, ix, iy, hIcon, iconSize, iconSize,
                           0, nullptr, DI_NORMAL);
            }
            else
            {
                // 没有自定义图标时，画一个简单的圆形提示
                HBRUSH hb = CreateSolidBrush(RGB(255, 255, 255));
                HBRUSH ob = (HBRUSH)SelectObject(hdc, hb);
                Ellipse(hdc, 10, 10, rc.right - 10, rc.bottom - 10);
                SelectObject(hdc, ob);
                DeleteObject(hb);
            }

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;  // 不需要擦除，减少闪烁

        case WM_LBUTTONDOWN:
        {
            s_dragging = true;
            s_moved    = false;
            SetCapture(hWnd);

            POINT pt;
            GetCursorPos(&pt);
            s_downPos = pt;

            RECT rc;
            GetWindowRect(hWnd, &rc);
            s_dragOffset.x = pt.x - rc.left;
            s_dragOffset.y = pt.y - rc.top;
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (s_dragging)
            {
                POINT pt;
                GetCursorPos(&pt);
                int dx = pt.x - s_downPos.x;
                int dy = pt.y - s_downPos.y;
                if (abs(dx) > 2 || abs(dy) > 2)
                    s_moved = true;

                SetWindowPos(hWnd, nullptr,
                             pt.x - s_dragOffset.x,
                             pt.y - s_dragOffset.y,
                             0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            bool wasClick = s_dragging && !s_moved;
            ReleaseCapture();
            s_dragging = false;
            s_moved    = false;
            if (wasClick)
                OpenWebPage();
            return 0;
        }

        case WM_RBUTTONUP:
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, IDM_OPEN_URL, L"打开网页");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT,     L"退出");
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON,
                           pt.x, pt.y, 0,
                           g_hHidden, nullptr);
            DestroyMenu(hMenu);
            return 0;
        }

        case WM_DESTROY:
            return 0;

        default:
            break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}
