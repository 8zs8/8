#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>

#define ID_TRAY_ICON        1001
#define IDM_OPEN_URL        2001
#define IDM_EXIT            2002
#define WM_TRAYMESSAGE      (WM_USER + 1)
#define ICON_RESOURCE_ID    1

static HINSTANCE g_hInstance   = nullptr;
static HWND      g_hWnd        = nullptr;
static NOTIFYICONDATA g_nid    = {};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    ShowTrayIcon(HWND hWnd);
void    RemoveTrayIcon();
void    OpenWebPage();

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE /*hPrevInstance*/,
                   _In_ LPSTR     /*lpCmdLine*/,
                   _In_ int       /*nShowCmd*/)
{
    g_hInstance = hInstance;

    // 注册主窗口类（窗口永不显示，仅作为托盘消息的接收者）
    const wchar_t CLASS_NAME[] = L"HiddenTrayWindowClass";

    WNDCLASSEXW wc     = {};
    wc.cbSize          = sizeof(wc);
    wc.lpfnWndProc     = WndProc;
    wc.hInstance       = hInstance;
    wc.lpszClassName   = CLASS_NAME;
    wc.hIcon           = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    wc.hIconSm         = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    wc.hCursor         = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"注册窗口类失败", L"错误", MB_ICONERROR);
        return 1;
    }

    // 创建一个隐藏窗口，仅用于接收托盘与任务栏消息
    g_hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"TrayLauncher",
        0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        HWND_MESSAGE,   // 仅消息窗口：不在任务栏、不显示
        nullptr,
        hInstance,
        nullptr);

    if (!g_hWnd)
    {
        MessageBoxW(nullptr, L"创建隐藏窗口失败", L"错误", MB_ICONERROR);
        return 1;
    }

    // 确保 LoadIcon 使用 hInstance 而不是 nullptr
    // 同时为 HWND_MESSAGE 窗口设置大/小图标，保证任务管理器显示自定义图标
    SendMessageW(g_hWnd, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));
    SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID)));

    if (!ShowTrayIcon(g_hWnd))
    {
        MessageBoxW(nullptr, L"创建托盘图标失败", L"错误", MB_ICONERROR);
        DestroyWindow(g_hWnd);
        return 1;
    }

    // 消息循环
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    RemoveTrayIcon();
    return (int)msg.wParam;
}

BOOL ShowTrayIcon(HWND hWnd)
{
    g_nid.cbSize           = sizeof(NOTIFYICONDATA);
    g_nid.hWnd             = hWnd;
    g_nid.uID              = ID_TRAY_ICON;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYMESSAGE;
    g_nid.hIcon            = LoadIconW(g_hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));

    if (!g_nid.hIcon)
        g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    StringCchCopyW(g_nid.szTip, ARRAYSIZE(g_nid.szTip), L"点击打开网页");

    return Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon()
{
    if (g_nid.hWnd)
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void OpenWebPage()
{
    const wchar_t URL[] = L"https://8zs8.github.io/8/";

    INT_PTR res = (INT_PTR)ShellExecuteW(nullptr,
                                         L"open",
                                         URL,
                                         nullptr,
                                         nullptr,
                                         SW_SHOWNORMAL);

    if (res <= 32)
    {
        MessageBoxW(nullptr, L"无法打开网页，请检查系统默认浏览器设置。",
                    L"提示", MB_ICONWARNING);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

                // 使菜单为顶层激活窗口，否则点击外部不会自动关闭
                SetForegroundWindow(hWnd);

                TrackPopupMenu(hMenu,
                               TPM_RIGHTBUTTON,
                               pt.x, pt.y,
                               0,
                               hWnd,
                               nullptr);

                DestroyMenu(hMenu);
                return 0;
            }

            return 0;
        }

        case WM_COMMAND:
        {
            WORD id = LOWORD(wParam);
            if (id == IDM_OPEN_URL)
                OpenWebPage();
            else if (id == IDM_EXIT)
                DestroyWindow(hWnd);
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
