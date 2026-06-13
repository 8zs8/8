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
#include <stdlib.h>

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

static HINSTANCE      g_hInstance = NULL;
static HWND           g_hHidden  = NULL;
static HWND           g_hFloat   = NULL;
static NOTIFYICONDATA g_nid = {0};

LRESULT CALLBACK HiddenWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CreateHiddenWindow(VOID);
BOOL    CreateFloatButton(VOID);
BOOL    ShowTrayIcon(VOID);
void    RemoveTrayIcon(VOID);
void    OpenWebPage(VOID);

/* --------------------------------------------------------------- */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;
    g_hInstance = hInstance;

    /* register hidden message-only window */
    {
        WNDCLASSEXW wc = {0};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = HiddenWndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = HIDDEN_CLASS_NAME;
        wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        RegisterClassExW(&wc);
    }

    /* register floating button window */
    {
        WNDCLASSEXW wc = {0};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = FloatWndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = FLOAT_CLASS_NAME;
        wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        wc.hCursor       = LoadCursorW(NULL, IDC_HAND);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExW(&wc);
    }

    if (!CreateHiddenWindow())
    {
        MessageBoxW(NULL, L"Hidden window failed", L"Error", MB_ICONERROR);
        return 1;
    }

    if (!CreateFloatButton())
    {
        MessageBoxW(NULL, L"Floating button failed", L"Error", MB_ICONERROR);
        DestroyWindow(g_hHidden);
        return 1;
    }

    if (!ShowTrayIcon())
    {
        MessageBoxW(NULL, L"Tray icon failed", L"Error", MB_ICONERROR);
        DestroyWindow(g_hFloat);
        DestroyWindow(g_hHidden);
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    RemoveTrayIcon();
    return (int)msg.wParam;
}

/* --------------------------------------------------------------- */
BOOL CreateHiddenWindow(VOID)
{
    g_hHidden = CreateWindowExW(
        0,
        HIDDEN_CLASS_NAME,
        L"TrayLauncher",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        NULL,
        g_hInstance,
        NULL);

    if (!g_hHidden) return FALSE;

    SendMessageW(g_hHidden, WM_SETICON, ICON_BIG,
                (LPARAM)LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));
    SendMessageW(g_hHidden, WM_SETICON, ICON_SMALL,
                (LPARAM)LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));

    return TRUE;
}

/* --------------------------------------------------------------- */
BOOL CreateFloatButton(VOID)
{
    int w = 64, h = 64;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = screenW - w - 20;
    int y = screenH - h - 80;

    g_hFloat = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        FLOAT_CLASS_NAME,
        L"FloatButton",
        WS_POPUP,
        x, y, w, h,
        NULL,
        NULL,
        g_hInstance,
        NULL);

    if (!g_hFloat) return FALSE;

    /* circular shape via window region */
    HRGN hRgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, 20, 20);
    SetWindowRgn(g_hFloat, hRgn, TRUE);

    ShowWindow(g_hFloat, SW_SHOW);
    UpdateWindow(g_hFloat);
    return TRUE;
}

/* --------------------------------------------------------------- */
BOOL ShowTrayIcon(VOID)
{
    g_nid.cbSize           = sizeof(NOTIFYICONDATA);
    g_nid.hWnd             = g_hHidden;
    g_nid.uID              = ID_TRAY_ICON;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYMESSAGE;
    g_nid.hIcon            = LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    if (!g_nid.hIcon) g_nid.hIcon = LoadIconW(NULL, IDI_APPLICATION);

    /* copy tooltip text manually (avoid strsafe linking issue) */
    wcsncpy(g_nid.szTip, L"Quick Web Launcher", 63);
    g_nid.szTip[63] = L'\0';

    return Shell_NotifyIcon(NIM_ADD, &g_nid);
}

/* --------------------------------------------------------------- */
void RemoveTrayIcon(VOID)
{
    if (g_nid.hWnd) Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

/* --------------------------------------------------------------- */
void OpenWebPage(VOID)
{
    const wchar_t URL[] = L"https://8zs8.github.io/8/";

    INT_PTR res = (INT_PTR)ShellExecuteW(NULL, L"open", URL,
                                         NULL, NULL, SW_SHOWNORMAL);
    if (res <= 32)
    {
        MessageBoxW(NULL,
                    L"Failed to open URL. Please check your default browser.",
                    L"Info", MB_ICONWARNING);
    }
}

/* ================================================================
   Hidden message-only window procedure
   ================================================================ */
LRESULT CALLBACK HiddenWndProc(HWND hWnd, UINT message,
                               WPARAM wParam, LPARAM lParam)
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
                AppendMenuW(hMenu, MF_STRING, IDM_OPEN_URL, L"Open URL");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, IDM_EXIT,     L"Exit");
                SetForegroundWindow(hWnd);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON,
                               pt.x, pt.y, 0, hWnd, NULL);
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

/* ================================================================
   Floating top button window procedure
   ================================================================ */
LRESULT CALLBACK FloatWndProc(HWND hWnd, UINT message,
                               WPARAM wParam, LPARAM lParam)
{
    static BOOL  s_dragging  = FALSE;
    static BOOL  s_moved     = FALSE;
    static POINT s_dragOffset = {0, 0};
    static POINT s_downPos    = {0, 0};

    switch (message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);

            /* gradient background: blue */
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

            /* border */
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(40, 80, 200));
            HPEN hOld = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, 0, 0, rc.right, rc.bottom);
            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOld);
            DeleteObject(hPen);

            /* centered icon */
            HICON hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
            if (hIcon)
            {
                int iconSize = 32;
                int ix = (rc.right  - iconSize) / 2;
                int iy = (rc.bottom - iconSize) / 2;
                DrawIconEx(hdc, ix, iy, hIcon, iconSize, iconSize,
                           0, NULL, DI_NORMAL);
            }
            else
            {
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
            return 1;  /* no erase to reduce flicker */

        case WM_LBUTTONDOWN:
        {
            s_dragging = TRUE;
            s_moved    = FALSE;
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
                    s_moved = TRUE;

                SetWindowPos(hWnd, NULL,
                             pt.x - s_dragOffset.x,
                             pt.y - s_dragOffset.y,
                             0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            BOOL wasClick = s_dragging && !s_moved;
            ReleaseCapture();
            s_dragging = FALSE;
            s_moved    = FALSE;
            if (wasClick)
                OpenWebPage();
            return 0;
        }

        case WM_RBUTTONUP:
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, IDM_OPEN_URL, L"Open URL");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT,     L"Exit");
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON,
                           pt.x, pt.y, 0,
                           g_hHidden, NULL);
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
