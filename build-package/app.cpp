// ====================================================================
//  Quick Web Launcher (main program) - app.cpp
//  ====================================================================
//
//  HOW TO COMPILE IN DEV-C++:
//
//  1. Open Dev-C++
//  2. File -> Open Project or File -> Open
//  3. Select: App.dev
//  4. Menu: Execute -> Compile  (or press Ctrl+F9)
//
//  IF YOU GET LINKER ERRORS:
//  Menu: Project -> Project Options -> Parameters tab
//  In the "Linker" box, paste this line:
//    -mwindows -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomctl32
//  Click OK, then press Ctrl+F9
//
//  ====================================================================

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

// ====================================================================
//  FORCE LINKER TO INCLUDE REQUIRED LIBRARIES
//  These #pragma directives tell the compiler to automatically link
//  these system libraries. If your MinGW version supports
//  #pragma comment(lib, ...) these will be linked automatically.
// ====================================================================
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "comctl32")
// Also try with .lib suffix (some MinGW versions)
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

// --- Forward declarations of our own functions. ---------------------
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);

#define ICON_RES_ID         1
#define FLOAT_WIN_CLASS     L"QWLFloatClass"
#define MSG_WIN_CLASS       L"QWLMsgClass"

static HWND g_hMsgWnd   = NULL;
static HWND g_hFloatWnd = NULL;
static BOOL g_dragging  = FALSE;
static int  g_dragDX    = 0;
static int  g_dragDY    = 0;

static const WCHAR g_targetUrl[] = L"https://8zs8.github.io/8/";

// ====================================================================
//  WinMain - entry point.
// ====================================================================
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // --- Register the message-only window class.
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
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                MessageBoxW(NULL,
                    L"Could not register the message window class.\n"
                    L"Please compile by opening App.dev (not app.cpp alone).",
                    L"Error", MB_ICONERROR | MB_OK);
                return 1;
            }
        }
    }

    // --- Register the floating-button window class.
    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc     = FloatWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = FLOAT_WIN_CLASS;
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wc.hIcon         = LoadIconW(hInstance,
                          MAKEINTRESOURCEW(ICON_RES_ID));
        if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                MessageBoxW(NULL,
                    L"Could not register the floating window class.\n"
                    L"Please compile by opening App.dev (not app.cpp alone).",
                    L"Error", MB_ICONERROR | MB_OK);
                return 1;
            }
        }
    }

    // --- Create message-only window (invisible, tray events go here).
    g_hMsgWnd = CreateWindowExW(
        0,
        MSG_WIN_CLASS,
        L"",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL);
    if (!g_hMsgWnd) {
        MessageBoxW(NULL, L"CreateWindowExW (message-only) failed.",
                  L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // --- Create the top-most floating button.
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int x = screenW - 120;
        int y = screenH - 180;
        if (x < 0) x = 32;
        if (y < 0) y = 32;

        g_hFloatWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            FLOAT_WIN_CLASS,
            L"",
            WS_POPUP,
            x, y, 88, 88,
            NULL, NULL, hInstance, NULL);

        if (!g_hFloatWnd) {
            MessageBoxW(NULL, L"CreateWindowExW (float) failed.",
                      L"Error", MB_ICONERROR | MB_OK);
            return 1;
        }

        // Apply rounded-corner shape.
        HRGN rgn = CreateRoundRectRgn(0, 0, 89, 89, 24, 24);
        if (rgn) SetWindowRgn(g_hFloatWnd, rgn, TRUE);

        ShowWindow(g_hFloatWnd, SW_SHOW);
        UpdateWindow(g_hFloatWnd);
        SetWindowPos(g_hFloatWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    // --- Add system tray icon.
    {
        NOTIFYICONDATAW nid;
        memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd   = g_hMsgWnd;
        nid.uID    = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER + 100;
        nid.hIcon  = LoadIconW(hInstance,
                            MAKEINTRESOURCEW(ICON_RES_ID));
        if (!nid.hIcon)
            nid.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
        // tooltip: "Quick Web Launcher" (up to 127 WCHARs).
        const wchar_t tip[] = L"Quick Web Launcher";
        int i;
        for (i = 0; i < 127 && tip[i]; i++) nid.szTip[i] = tip[i];
        nid.szTip[i] = 0;

        Shell_NotifyIconW(NIM_ADD, &nid);
    }

    // --- Standard message loop.
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // --- Cleanup.
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
//  Open the target URL with the default browser.
// ====================================================================
static void OpenTargetUrl(void)
{
    HINSTANCE r = ShellExecuteW(NULL, L"open", g_targetUrl,
                                 NULL, NULL, SW_SHOWNORMAL);
    if (!r || (INT_PTR)r <= 32) {
        MessageBoxW(NULL, L"Could not open the URL.",
                  L"Info", MB_ICONINFORMATION | MB_OK);
    }
}

// ====================================================================
//  Message-only window procedure - handles tray-icon events.
// ====================================================================
LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg,
                             WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_USER + 100:
        {
            UINT mouseMsg = (UINT)lParam;
            if (mouseMsg == WM_LBUTTONDOWN ||
                mouseMsg == WM_LBUTTONDBLCLK)
            {
                OpenTargetUrl();
                return 0;
            }
            if (mouseMsg == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu, MF_STRING, 1001, L"Open URL");
                AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(menu, MF_STRING, 1002, L"Exit");
                UINT clicked = TrackPopupMenu(
                    menu,
                    TPM_RETURNCMD | TPM_RIGHTBUTTON,
                    pt.x, pt.y, 0, hwnd, NULL);
                if (clicked == 1001) OpenTargetUrl();
                else if (clicked == 1002) {
                    DestroyWindow(g_hFloatWnd);
                    DestroyWindow(hwnd);
                }
                DestroyMenu(menu);
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
//  Floating button window procedure.
// ====================================================================
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

            // Draw vertical gradient fill (blue).
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

            // Draw centered icon.
            HICON hIcon = LoadIconW(
                    (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
                    MAKEINTRESOURCEW(ICON_RES_ID));
            if (!hIcon) hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
            if (hIcon) {
                DrawIconEx(hdc, 20, 20, hIcon, 48, 48, 0, NULL, DI_NORMAL);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
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
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, 1001, L"Open URL");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(menu, MF_STRING, 1002, L"Exit");
            UINT clicked = TrackPopupMenu(
                menu,
                TPM_RETURNCMD | TPM_RIGHTBUTTON,
                pt.x, pt.y, 0, hwnd, NULL);
            if (clicked == 1001) OpenTargetUrl();
            else if (clicked == 1002) {
                DestroyWindow(g_hFloatWnd);
                if (g_hMsgWnd) DestroyWindow(g_hMsgWnd);
            }
            DestroyMenu(menu);
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
