// app.cpp - Quick Web Launcher tray + top-floating button
// ASCII-only, no Chinese text, works on any Windows codepage.

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
#include <stdio.h>

// Link required system libs
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

#define ICON_RESOURCE_ID   1
#define ID_TRAY_OPEN_URL   40001
#define ID_TRAY_EXIT        40002
#define BUTTON_SIZE         72
#define WNDCLASSNAME       L"QuickWebLauncher_FloatBtn_Class"

// ---------- Logging helper (to %TEMP%\qwl_log.txt) ----------
static void log_write(const wchar_t* fmt, ...)
{
    wchar_t path[MAX_PATH];
    wchar_t buf[512];
    va_list args;
    va_start(args, fmt);
    wvsprintfW(buf, fmt, args);
    va_end(args);
    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"qwl_log.txt");

    HANDLE h = CreateFileW(path, FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(h, buf, (DWORD)(lstrlenW(buf) * sizeof(wchar_t)),
              &written, NULL);
    WriteFile(h, L"\r\n", 4, &written, NULL);
    CloseHandle(h);
}

// ---------- Globals ----------
static HWND      g_hwnd_float = NULL;
static HWND      g_hwnd_msg   = NULL;
static NOTIFYICONDATAA g_nid;
static HINSTANCE   g_hInstance = NULL;
static int         g_screen_w = 0;
static int         g_screen_h = 0;
static int         g_drag_dx = 0;
static int         g_drag_dy = 0;
static BOOL        g_dragging = FALSE;

static const wchar_t k_target_url[] =
    L"https://8zs8.github.io/8/";

// ---------- Forward decls ----------
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CreateMsgWindow(void);
BOOL    CreateFloatButton(void);
BOOL    AddTrayIcon(void);
void    RemoveTrayIcon(void);
void    OpenTargetUrl(void);

// ============================================================
//  WinMain
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

    log_write(L"WinMain starting...");

    // -- screen size --
    g_screen_w = GetSystemMetrics(SM_CXSCREEN);
    g_screen_h = GetSystemMetrics(SM_CYSCREEN);
    log_write(L"screen: %d x %d", g_screen_w, g_screen_h);

    // -- register window class for floating button --
    {
        WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc     = FloatWndProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconW(hInstance,
                          MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    if (!wc.hIcon)
        wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursorW(NULL, IDC_HAND);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WNDCLASSNAME;
    wc.style          = CS_HREDRAW | CS_VREDRAW;

    ATOM at = RegisterClassExW(&wc);
    log_write(L"Float RegisterClassEx returned %u (class registered)", at);
    }

    // -- hidden message window (for tray icon callbacks) --
    {
        WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc     = MsgWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"QWLMsgClass";
    RegisterClassExW(&wc);
    }

    // 1) Create message window
    if (!CreateMsgWindow())
    {
        log_write(L"FATAL: CreateMsgWindow failed");
        MessageBoxW(NULL, L"Failed to create message window",
                      L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // 2) Create floating button (top-right of screen)
    if (!CreateFloatButton())
    {
        log_write(L"FATAL: CreateFloatButton failed");
        MessageBoxW(NULL, L"Failed to create floating button",
                  L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // 3) Add tray icon
    if (!AddTrayIcon())
    {
        log_write(L"WARN: AddTrayIcon failed (continuing anyway)");
        // non-fatal: app still runs with floating button
    }

    log_write(L"Entering message loop");

    // Standard message loop.
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    log_write(L"Exiting, last_msg=%d", (int)msg.message);
    return (int)msg.wParam;
}

// ============================================================
//  Floating button window - always on top
// ============================================================
BOOL CreateFloatButton(void)
{
    // bottom-right corner, with small offset from edge.
    int x = g_screen_w - BUTTON_SIZE - 40;
    int y = g_screen_h - BUTTON_SIZE - 60;
    if (x < 0) x = 10;
    if (y < 0) y = 10;

    g_hwnd_float = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        WNDCLASSNAME,
        L"Quick Web Launcher",
        WS_POPUP | WS_VISIBLE,
        x, y, BUTTON_SIZE, BUTTON_SIZE,
        NULL, NULL, g_hInstance, NULL);

    if (!g_hwnd_float) {
        log_write(L"CreateWindowExW failed, error=%lu", GetLastError());
        return FALSE;
    }

    // Round-corner shape (SetWindowPos(hwnd, HWND_TOPMOST, x, y, BUTTON_SIZE, BUTTON_SIZE, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    SetWindowRgn(g_hwnd_float,
                    CreateRoundRectRgn(0, 0, BUTTON_SIZE, BUTTON_SIZE, 16, 16), TRUE);
    ShowWindow(g_hwnd_float, SW_SHOW);
    UpdateWindow(g_hwnd_float);
    SetWindowPos(g_hwnd_float, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

    log_write(L"Float button created at (%d,%d)", x, y);
    return TRUE;
}

// ============================================================
//  Message window (hidden; for tray icon notifications)
// ============================================================
BOOL CreateMsgWindow(void)
{
    g_hwnd_msg = CreateWindowExW(
        0, L"QWLMsgClass", L"", 0,
        0, 0, 0, 0,
        HWND_MESSAGE, NULL, g_hInstance, NULL);
    return g_hwnd_msg != NULL;
}

// ============================================================
//  Tray icon
// ============================================================
BOOL AddTrayIcon(void)
{
    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwnd_msg;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_USER + 1;
    g_nid.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    if (!g_nid.hIcon)
        g_nid.hIcon = LoadIconW(NULL, IDI_INFORMATION);
    // tooltip text
    const wchar_t* tip = L"Quick Web Launcher - click to open URL";
    size_t tip_len = wcslen(tip);
    if (tip_len >= 64) tip_len = 63;
    wcsncpy(g_nid.szTip, tip, tip_len);
    g_nid.szTip[tip_len] = 0;

    BOOL ok = Shell_NotifyIconW(NIM_ADD, &g_nid);
    log_write(L"Shell_NotifyIconW(NIM_ADD) returned %d", (int)ok);
    return ok;
}

void RemoveTrayIcon(void)
{
    if (g_hwnd_msg) {
        memset(&g_nid, 0, sizeof(g_nid));
        g_nid.cbSize = sizeof(g_nid);
        g_nid.hWnd = g_hwnd_msg;
        g_nid.uID = 1;
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
    }
}

// ============================================================
//  Open the target URL in default browser
// ============================================================
void OpenTargetUrl(void)
{
    HINSTANCE r = ShellExecuteW(NULL, L"open", k_target_url,
                                   NULL, NULL, SW_SHOWNORMAL);
    log_write(L"ShellExecuteW returned %p", (void*)r);
    // According to Microsoft, return value <= 32 means error.
    if ((INT_PTR)r <= 32) {
        wchar_t msg[256];
        wvsprintfW(msg, L"Failed to open URL (err=0x%Ix)",
                    NULL);
        MessageBoxW(NULL, msg, L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
}

// ============================================================
//  Floating button window proc
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

            // draw circular gradient fill (blue)
            TRIVERTEX vtx[2];
            memset(vtx, 0, sizeof(vtx));
            vtx[0].x      = rc.left;
            vtx[0].y      = rc.top;
            vtx[0].Red    = 0x40 << 8;
            vtx[0].Green  = 0x80 << 8;
            vtx[0].Blue   = 0xE0 << 8;
            vtx[0].Alpha  = 0xFF << 8;
            vtx[1].x      = rc.right;
            vtx[1].y      = rc.bottom;
            vtx[1].Red    = 0x70 << 8;
            vtx[1].Green  = 0xB0 << 8;
            vtx[1].Blue   = 0xFF << 8;
            vtx[1].Alpha = 0xFF << 8;

            GRADIENT_RECT grect;
    grect.UpperLeft = 0;
    grect.LowerRight = 1;

            GradientFill(hdc, vtx, 2, &grect, 1, GRADIENT_FILL_RECT_V);

            // draw centered icon
            HICON hIcon = LoadIconW(g_hInstance,
                                 MAKEINTRESOURCEW(ICON_RESOURCE_ID));
            if (!hIcon) hIcon = LoadIconW(NULL, IDI_INFORMATION);
            if (hIcon) {
                int size = 32;
                DrawIconEx(hdc, (BUTTON_SIZE-size)/2, (BUTTON_SIZE-size)/2,
                           hIcon, size, size, 0, NULL, DI_NORMAL);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            // Start drag. Save delta between window top-left and cursor.
            POINT pt;
    GetCursorPos(&pt);
            RECT rc;
    GetWindowRect(hwnd, &rc);
            g_drag_dx = pt.x - rc.left;
            g_drag_dy = pt.y - rc.top;
            g_dragging = TRUE;
            SetCapture(hwnd);
            // also register that we are drag;
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            if (g_dragging)
            {
                POINT pt;
                GetCursorPos(&pt);
                SetWindowPos(hwnd, HWND_TOPMOST,
                            pt.x - g_drag_dx, pt.y - g_drag_dy,
                            0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            }
            return 0;
        }
        case WM_LBUTTONUP:
        {
            if (g_dragging)
            {
                ReleaseCapture();
                g_dragging = FALSE;
                // treat as click -> open URL.
                OpenTargetUrl();
            }
            return 0;
        }
        case WM_RBUTTONUP:
        {
            // right-click: open URL menu or exit
            POINT pt; GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_URL, L"Open URL");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
            SetForegroundWindow(hwnd);
            UINT clicked = TrackPopupMenu(
                hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                pt.x, pt.y, 0, hwnd, NULL);
            if (clicked == ID_TRAY_OPEN_URL) OpenTargetUrl();
            else if (clicked == ID_TRAY_EXIT) PostQuitMessage(0);
            DestroyMenu(hMenu);
            return 0;
        }

        case WM_DESTROY:
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================
//  Message window proc (tray icon events + menu commands)
// ============================================================
LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg,
                              WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_USER + 1: // tray callback
        {
            UINT mouseMsg = (UINT)lParam;
            if (mouseMsg == WM_LBUTTONDOWN || mouseMsg == WM_LBUTTONDBLCLK)
            {
                OpenTargetUrl();
                return 0;
            }
            if (mouseMsg == WM_RBUTTONUP)
            {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_URL, L"Open URL");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
                SetForegroundWindow(hwnd);
                UINT clicked = TrackPopupMenu(
                    hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                    pt.x, pt.y, 0, hwnd, NULL);
                if (clicked == ID_TRAY_OPEN_URL) OpenTargetUrl();
                else if (clicked == ID_TRAY_EXIT) PostQuitMessage(0);
                DestroyMenu(hMenu);
                return 0;
            }
            return 0;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case ID_TRAY_OPEN_URL: OpenTargetUrl(); return 0;
                case ID_TRAY_EXIT: PostQuitMessage(0); return 0;
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
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
