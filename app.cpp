// app.cpp - Quick Web Launcher tray + top-most floating button
// ASCII-only source. Uses Unicode (W) APIs only.
//
// Build:
//   windres -o app_res.o app.rc
//   g++ -O2 -mwindows -o app.exe app.cpp app_res.o \
//       -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32

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

// required link libraries
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

#define ICON_RESOURCE_ID       1
#define FLOAT_CLASS_NAME       L"QWLFloatClass"
#define MSG_CLASS_NAME         L"QWLMsgClass"
#define FLOAT_BTN_ID           6001
#define FLOAT_BTN_WIDTH        88
#define FLOAT_BTN_HEIGHT       88

static HINSTANCE g_hInstance  = NULL;
static HWND      g_hMsgWnd   = NULL;
static HWND      g_hFloatWnd = NULL;
static BOOL      g_dragging  = FALSE;
static int       g_dragDX    = 0;
static int       g_dragDY    = 0;

static const WCHAR g_target_url[] =
    L"https://8zs8.github.io/8/";

// ---- forward declarations ----
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CreateMessageWindow(void);
BOOL    CreateFloatButton(void);
BOOL    AddTrayIcon(void);
void    RemoveTrayIcon(void);
void    OpenTargetUrl(void);

// ============================================================
//  WinMain - entry point
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    g_hInstance = hInstance;

    // register the message-only window class (for tray callbacks)
    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc     = MsgWndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = MSG_CLASS_NAME;
        wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                MessageBoxW(NULL, L"Failed to register message window class.",
                          L"Fatal", MB_ICONERROR | MB_OK);
                return 1;
            }
        }
    }

    // register the floating-button window class
    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc     = FloatWndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = FLOAT_CLASS_NAME;
        wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wc.hIcon         = LoadIconW(hInstance,
                          MAKEINTRESOURCEW(ICON_RESOURCE_ID));
        if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                MessageBoxW(NULL, L"Failed to register floating window class.",
                          L"Fatal", MB_ICONERROR | MB_OK);
                return 1;
            }
        }
    }

    if (!CreateMessageWindow()) {
        MessageBoxW(NULL, L"CreateMessageWindow failed",
                  L"Fatal", MB_ICONERROR | MB_OK);
        return 1;
    }

    if (!CreateFloatButton()) {
        MessageBoxW(NULL, L"CreateFloatButton failed",
                  L"Fatal", MB_ICONERROR | MB_OK);
        DestroyWindow(g_hMsgWnd);
        return 1;
    }

    AddTrayIcon();

    // standard message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    RemoveTrayIcon();
    return (int)msg.wParam;
}

// ============================================================
//  Create a hidden message-only window (for tray icon callbacks)
// ============================================================
BOOL CreateMessageWindow(void)
{
    g_hMsgWnd = CreateWindowExW(
        0,
        MSG_CLASS_NAME,
        L"",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,   // message-only window: invisible, no taskbar
        NULL,
        g_hInstance,
        NULL);
    return (g_hMsgWnd != NULL);
}

// ============================================================
//  Create the floating top-most button
// ============================================================
BOOL CreateFloatButton(void)
{
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = screenW - FLOAT_BTN_WIDTH - 32;
    int y = screenH - FLOAT_BTN_HEIGHT - 80;
    if (x < 0) x = 16;
    if (y < 0) y = 16;

    g_hFloatWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        FLOAT_CLASS_NAME,
        L"QWL Float",
        WS_POPUP,
        x, y, FLOAT_BTN_WIDTH, FLOAT_BTN_HEIGHT,
        NULL, NULL, g_hInstance, NULL);

    if (!g_hFloatWnd) {
        return FALSE;
    }

    // apply rounded-corner shape
    HRGN rgn = CreateRoundRectRgn(0, 0,
                                 FLOAT_BTN_WIDTH + 1,
                                 FLOAT_BTN_HEIGHT + 1,
                                 24, 24);
    SetWindowRgn(g_hFloatWnd, rgn, TRUE);

    ShowWindow(g_hFloatWnd, SW_SHOW);
    UpdateWindow(g_hFloatWnd);

    // force top-most
    SetWindowPos(g_hFloatWnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    return TRUE;
}

// ============================================================
//  Tray icon (using WIDE struct)
// ============================================================
static NOTIFYICONDATAW g_nid; // Unicode version of the struct

BOOL AddTrayIcon(void)
{
    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd   = g_hMsgWnd;
    g_nid.uID    = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_USER + 100;
    g_nid.hIcon  = LoadIconW(g_hInstance,
                           MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    if (!g_nid.hIcon)
        g_nid.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);

    // copy tooltip text (WCHAR strings)
    const wchar_t* tip_text = L"Quick Web Launcher";
    int i;
    for (i = 0; i < 127 && tip_text[i]; i++) {
        g_nid.szTip[i] = tip_text[i];
    }
    g_nid.szTip[i] = 0;

    BOOL ok = Shell_NotifyIconW(NIM_ADD, &g_nid);
    return ok;
}

void RemoveTrayIcon(void)
{
    if (g_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
    }
}

// ============================================================
//  Launch the target URL with default browser
// ============================================================
void OpenTargetUrl(void)
{
    HINSTANCE r = ShellExecuteW(NULL, L"open", g_target_url,
                                   NULL, NULL, SW_SHOWNORMAL);
    if (!r || (INT_PTR)r <= 32) {
        MessageBoxW(NULL, L"Could not open the target URL.",
                  L"Info", MB_ICONINFORMATION | MB_OK);
    }
}

// ============================================================
//  Message-only window procedure (handles tray events)
// ============================================================
LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg,
                             WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_USER + 100:
        {
            UINT mouse = (UINT)lParam;
            if (mouse == WM_LBUTTONDOWN || mouse == WM_LBUTTONDBLCLK) {
                OpenTargetUrl();
            }
            else if (mouse == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, 1, L"Open URL");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, 2, L"Exit");
                UINT clicked = TrackPopupMenu(
                    hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                    pt.x, pt.y, 0, hwnd, NULL);
                if (clicked == 1) OpenTargetUrl();
                else if (clicked == 2) {
                    RemoveTrayIcon();
                    DestroyWindow(g_hFloatWnd);
                    DestroyWindow(hwnd);
                }
                DestroyMenu(hMenu);
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================
//  Floating top-most button window procedure
// ============================================================
LRESULT CALLBACK FloatWndProc(HWND hwnd, UINT msg,
                               WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            TRIVERTEX vtx[2];
            memset(&vtx, 0, sizeof(vtx));
            vtx[0].x     = rc.left;
            vtx[0].y     = rc.top;
            vtx[0].Red   = 0x50 << 8;
            vtx[0].Green = 0x90 << 8;
            vtx[0].Blue  = 0xF0 << 8;
            vtx[0].Alpha = 0xFF << 8;
            vtx[1].x     = rc.right;
            vtx[1].y     = rc.bottom;
            vtx[1].Red   = 0x25 << 8;
            vtx[1].Green = 0x55 << 8;
            vtx[1].Blue  = 0xD5 << 8;
            vtx[1].Alpha = 0xFF << 8;

            GRADIENT_RECT grect;
            grect.UpperLeft  = 0;
            grect.LowerRight = 1;
            GradientFill(hdc, vtx, 2, &grect, 1, GRADIENT_FILL_RECT_V);

            // draw centered icon
            HICON hIcon = LoadIconW(g_hInstance,
                            MAKEINTRESOURCEW(ICON_RESOURCE_ID));
            if (!hIcon) hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
            if (hIcon) {
                int size = 48;
                int cx = (FLOAT_BTN_WIDTH - size) / 2;
                int cy = (FLOAT_BTN_HEIGHT - size) / 2;
                DrawIconEx(hdc, cx, cy, hIcon, size, size,
                          0, NULL, DI_NORMAL);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            // start drag: capture mouse, save offset
            g_dragging = TRUE;
            SetCapture(hwnd);
            POINT pt;
            GetCursorPos(&pt);
            RECT rc;
            GetWindowRect(hwnd, &rc);
            g_dragDX = pt.x - rc.left;
            g_dragDY = pt.y - rc.top;
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (g_dragging) {
                POINT pt;
                GetCursorPos(&pt);
                SetWindowPos(hwnd, HWND_TOPMOST,
                            pt.x - g_dragDX, pt.y - g_dragDY,
                            0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            if (g_dragging) {
                ReleaseCapture();
                g_dragging = FALSE;
                OpenTargetUrl();
            }
            return 0;
        }

        case WM_RBUTTONUP:
        {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"Open URL");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, 2, L"Exit");
            UINT clicked = TrackPopupMenu(
                hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                pt.x, pt.y, 0, hwnd, NULL);
            if (clicked == 1) OpenTargetUrl();
            else if (clicked == 2) {
                RemoveTrayIcon();
                DestroyWindow(g_hFloatWnd);
                if (g_hMsgWnd) DestroyWindow(g_hMsgWnd);
            }
            DestroyMenu(hMenu);
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
