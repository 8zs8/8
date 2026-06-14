// ====================================================================
//  Quick Web Launcher (main program) - app.cpp
//  Program: 抽人软件
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
// ====================================================================

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
//  FORCE GUI SUBSYSTEM (no console / black window at runtime).
//  -mwindows tells the linker this is a GUI app, not a console app.
//  The GCC pragma below embeds this flag in the object file so it
//  takes effect even if the Dev-C++ project forgot to pass it.
// ====================================================================
#ifdef __GNUC__
#pragma comment(linker, "-mwindows")
#endif

// ====================================================================
//  FORCE LINKER TO INCLUDE REQUIRED LIBRARIES.
//  NOTE: #pragma comment(lib, ...) requires MinGW GCC 4.5+ with
//  linker directive support. Library names must be WITHOUT .lib suffix.
//  If your MinGW version does not support this, add -lxxx flags
//  through the Dev-C++ Project Options GUI (see file header).
// ====================================================================
#ifdef __GNUC__
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "comctl32")
#endif

// --- Forward declarations of our own functions. ---------------------
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FloatWndProc(HWND, UINT, WPARAM, LPARAM);
static void OpenTargetUrl(void);
static void ShowContextMenu(HWND hwnd);

// --- Resource IDs. --------------------------------------------------
#define ICON_RES_ID         1

// --- Window class names. --------------------------------------------
#define FLOAT_WIN_CLASS     L"QWLFloatClass"
#define MSG_WIN_CLASS       L"QWLMsgClass"

// --- Timer ID for long-press detection. -----------------------------
#define LONGPRESS_TIMER_ID  1001
#define LONGPRESS_MS        500       // hold >=0.5 s = "right-click"
#define DRAG_THRESHOLD_PX   5         // >5 px movement = drag, not click

// --- Floating-button geometry. --------------------------------------
// The floating window is now small and only shows the icon (no
// background fill). The color-key transparency makes everything
// except the icon-pixels on the client area fully transparent.
#define FLOAT_SIZE          56        // window is 56x56
#define ICON_DRAW_SIZE      48        // icon rendered at 48x48, centered
#define ICON_OFFSET_X       4         // (56-48)/2 = 4
#define ICON_OFFSET_Y       4

// --- Color key used for layered transparency. -----------------------
// Magenta is unlikely to appear in any real icon bitmap. Everything
// painted with this exact color will become 100% transparent.
#define COLORKEY_R         255
#define COLORKEY_G         0
#define COLORKEY_B         255
static const COLORREF COLORKEY = RGB(COLORKEY_R, COLORKEY_G, COLORKEY_B);

// --- Global state. --------------------------------------------------
static HWND g_hMsgWnd    = NULL;
static HWND g_hFloatWnd  = NULL;

static BOOL g_pressed    = FALSE;   // left button currently down
static BOOL g_dragged    = FALSE;   // movement >= DRAG_THRESHOLD_PX
static BOOL g_longpressed=FALSE;    // long-press timer fired before up
static int  g_pressX     = 0;       // screen X at WM_LBUTTONDOWN
static int  g_pressY     = 0;       // screen Y at WM_LBUTTONDOWN
static int  g_dragDX     = 0;
static int  g_dragDY     = 0;

// --- Target URL. ----------------------------------------------------
static const WCHAR g_targetUrl[] = L"https://8zs8.github.io/8/";
static const WCHAR g_appName[]   = L"抽人软件";

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
        wc.hbrBackground = (HBRUSH)NULL;              // no background
        wc.lpszClassName = MSG_WIN_CLASS;
        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                MessageBoxW(NULL,
                    L"Could not register the message window class.\n"
                    L"Please compile by opening App.dev (not app.cpp alone).",
                    g_appName, MB_ICONERROR | MB_OK);
                return 1;
            }
        }
    }

    // --- Register the floating-button window class.
    {
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc   = FloatWndProc;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
        wc.hbrBackground = (HBRUSH)NULL;               // no background
        wc.lpszClassName = FLOAT_WIN_CLASS;
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wc.hIcon          = LoadIconW(hInstance,
                            MAKEINTRESOURCEW(ICON_RES_ID));
        if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                MessageBoxW(NULL,
                    L"Could not register the floating window class.\n"
                    L"Please compile by opening App.dev (not app.cpp alone).",
                    g_appName, MB_ICONERROR | MB_OK);
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
                  g_appName, MB_ICONERROR | MB_OK);
        return 1;
    }

    // --- Create the top-most floating button.
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int x = screenW - 100;
        int y = screenH - 150;
        if (x < 0) x = 32;
        if (y < 0) y = 32;

        // WS_EX_LAYERED is required for color-key transparency.
        g_hFloatWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
            FLOAT_WIN_CLASS,
            g_appName,                         // window title = program name
            WS_POPUP,
            x, y, FLOAT_SIZE, FLOAT_SIZE,
            NULL, NULL, hInstance, NULL);

        if (!g_hFloatWnd) {
            MessageBoxW(NULL, L"CreateWindowExW (float) failed.",
                      g_appName, MB_ICONERROR | MB_OK);
            return 1;
        }

        // --- Apply color-key transparency.
        // Every pixel painted COLORKEY_RGB becomes fully transparent.
        SetLayeredWindowAttributes(g_hFloatWnd,
                                   COLORKEY, 0, LWA_COLORKEY);

        // --- Use a rounded region for the window so even the color-key
        // "frame" around the icon doesn't respond to clicks.
        HRGN rgn = CreateRoundRectRgn(0, 0,
                            FLOAT_SIZE + 1, FLOAT_SIZE + 1,
                            FLOAT_SIZE, FLOAT_SIZE);
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

        // Tray tooltip: "抽人软件"
        {
            const wchar_t *tip = g_appName;
            int i;
            for (i = 0; i < 127 && tip[i]; i++) nid.szTip[i] = tip[i];
            nid.szTip[i] = 0;
        }

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
                  g_appName, MB_ICONINFORMATION | MB_OK);
    }
}

// ====================================================================
//  Show a right-click context menu for the floating button.
// ====================================================================
static void ShowContextMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, L"打开网页");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, 1002, L"退出");

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

            // Single left-click on tray icon -> open URL.
            if (mouseMsg == WM_LBUTTONDOWN ||
                mouseMsg == WM_LBUTTONDBLCLK)
            {
                OpenTargetUrl();
                return 0;
            }

            // Right-click on tray icon -> show context menu.
            if (mouseMsg == WM_RBUTTONUP) {
                ShowContextMenu(hwnd);
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
//
//  Interaction model (this is what we changed):
//
//   * WM_LBUTTONDOWN  -> record position, start long-press timer,
//                        capture mouse, mark "pressed".
//   * WM_TIMER        -> if still pressed AND not yet dragged,
//                        treat as long-press ("right-click").
//   * WM_MOUSEMOVE    -> if pressed + movement exceeds threshold,
//                        mark as "dragged", kill long-press timer,
//                        start moving the window.  (No URL open.)
//   * WM_LBUTTONUP    -> if not dragged and not long-pressed,
//                        this was a clean single CLICK -> open URL.
//                        Otherwise do nothing (the drag/long-press
//                        handler already took care of it).
//   * WM_RBUTTONUP    -> show context menu (standard right-click).
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

            // Fill the entire client area with the color-key color.
            // Every pixel painted with COLORKEY becomes 100%
            // transparent (invisible), so there is no background.
            HBRUSH hBrushKey = CreateSolidBrush(COLORKEY);
            FillRect(hdc, &rc, hBrushKey);
            DeleteObject(hBrushKey);

            // Draw the user's icon centered, at 48x48. DrawIconEx
            // preserves the icon's own transparency (alpha).
            HICON hIcon = LoadIconW(
                    (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
                    MAKEINTRESOURCEW(ICON_RES_ID));
            if (!hIcon) hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
            if (hIcon) {
                DrawIconEx(hdc, ICON_OFFSET_X, ICON_OFFSET_Y,
                           hIcon, ICON_DRAW_SIZE, ICON_DRAW_SIZE,
                           0, NULL, DI_NORMAL);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            // --- Record initial press state.
            g_pressed       = TRUE;
            g_dragged       = FALSE;
            g_longpressed   = FALSE;

            POINT pt;
            GetCursorPos(&pt);
            g_pressX = pt.x;
            g_pressY = pt.y;

            RECT rc;
            GetWindowRect(hwnd, &rc);
            g_dragDX = pt.x - rc.left;
            g_dragDY = pt.y - rc.top;

            // --- Capture mouse so we receive WM_MOUSEMOVE / WM_LBUTTONUP
            // even if the pointer leaves the window.
            SetCapture(hwnd);

            // --- Start the long-press timer.
            // If the user holds the button without dragging for
            // LONGPRESS_MS, we fire "long-press == right-click".
            SetTimer(hwnd, LONGPRESS_TIMER_ID, LONGPRESS_MS, NULL);

            return 0;
        }

        case WM_TIMER:
        {
            if (wParam == LONGPRESS_TIMER_ID && g_pressed) {
                // --- Long-press fired: treat as right-click,
                // kill timer, prevent WM_LBUTTONUP from opening URL.
                KillTimer(hwnd, LONGPRESS_TIMER_ID);
                g_longpressed = TRUE;

                // --- Release mouse/capture so the context menu can
                // receive input normally.
                if (GetCapture() == hwnd) ReleaseCapture();
                g_pressed = FALSE;

                ShowContextMenu(hwnd);
            }
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (!g_pressed) return 0;

            POINT pt;
            GetCursorPos(&pt);

            // --- Check if movement has exceeded the drag threshold.
            if (!g_dragged) {
                int dx = pt.x - g_pressX;
                int dy = pt.y - g_pressY;
                if (dx < 0) dx = -dx;
                if (dy < 0) dy = -dy;
                if (dx > DRAG_THRESHOLD_PX || dy > DRAG_THRESHOLD_PX) {
                    // --- This is now a drag.  Cancel the long-press
                    // timer so holding + dragging does not fire a
                    // spurious right-click.
                    KillTimer(hwnd, LONGPRESS_TIMER_ID);
                    g_dragged = TRUE;
                }
            }

            // --- If in drag mode, actually move the window.
            if (g_dragged) {
                SetWindowPos(hwnd, HWND_TOPMOST,
                            pt.x - g_dragDX, pt.y - g_dragDY,
                            0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            if (!g_pressed) return 0;

            // --- Clean up.
            if (GetCapture() == hwnd) ReleaseCapture();
            KillTimer(hwnd, LONGPRESS_TIMER_ID);
            BOOL wasDragged      = g_dragged;
            BOOL wasLongPressed  = g_longpressed;
            g_pressed = FALSE;
            g_dragged = FALSE;
            g_longpressed = FALSE;

            // --- Only a "clean" (no drag, no long-press) click
            // should open the URL.  Dragging must NOT open the page.
            if (!wasDragged && !wasLongPressed) {
                OpenTargetUrl();
            }
            // --- If it WAS a drag or a long-press: do nothing here.
            // (Drag just repositions the button; long-press already
            // showed a context menu inside WM_TIMER.)
            return 0;
        }

        case WM_RBUTTONUP:
        {
            // --- Normal right-click: show context menu.
            ShowContextMenu(hwnd);
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
