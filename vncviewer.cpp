﻿// vncviewer.cpp : 定义应用程序的入口点。
//
#define _CRT_SECURE_NO_WARNINGS
#define CONSOLE_OUTPUT
#include "framework.h"
#include "cmdline.h"
#include "ClientConnection.h"
#include "clipboard.h"
#include "common_utils.h"
#include "vncviewer.h"
#include <iostream>
#include <queue>
#include "vncKeymap.h"
#include "pointerProc.h"
#include "work_thread.h"
#include <Windows.h>
#include <utility>
#include "bitmap.h"
#include <wingdi.h>

#pragma comment(lib, "Msimg32.lib") // gdi
#pragma comment(lib, "libyuv_internal.lib")
#define MAX_LOADSTRING 100



// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

DWORD thdID = 0;

bool captureCursor = true;
HWND hWnd = nullptr;
HWND hWndChild = nullptr;
HWND hwnd_next_viewer = nullptr;
HWND message_wnd_ = nullptr;
ClientControlCaptureImpl* pcc = nullptr;
HCURSOR g_h1 = nullptr;
HBITMAP bmp = nullptr;
std::queue<rfbPointerEventMsg> pointerQ;
std::queue<rfbKeyEventMsg> keyQ;
std::queue<HCURSOR> hcursorQ;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DeviceProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndChildProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void ShowKeyboard();
void ShowPointer();
void ShowCursor();
void CreateChildWindow();

DWORD WINAPI CaptureCursorBitmap(PVOID pvParam)
{
    Sleep(1000);
    printf_s("\ncapture cursor shape icon started! \n");
    int sample_rate = 1000;
    int64_t last_time = 0;
    int64_t per_sample_duration = (int64_t)(1000000.0 / (double)sample_rate + 0.5);
    int last_sample_rate = sample_rate;

    HANDLE timer_handle = CreateWaitableTimer(NULL, FALSE, NULL);
    if (NULL == timer_handle)
    {
        printf_s("timer_handle create failed! \n");
        return -1;
    }
    
    int64_t start = TimeMilliseconds();
    int cnt = 0;
    while (captureCursor)
    {
        if (last_sample_rate != sample_rate)
        {
            per_sample_duration = (int64_t)(1000000.0 / (double)sample_rate + 0.5);
            last_sample_rate = sample_rate;
        }

        int64_t cur_time = TimeMicroseconds();
        int64_t last_spend_time = cur_time - last_time;
        
        
        if (last_spend_time < per_sample_duration)
        {
            int64_t cur_time = TimeMicroseconds();
            LARGE_INTEGER liDueTime;
            liDueTime.QuadPart = -(per_sample_duration - last_spend_time) * 10;
            SetWaitableTimer(timer_handle, &liDueTime, 0, NULL, NULL, 0);
            if (WaitForSingleObject(timer_handle, INFINITE) != WAIT_OBJECT_0)
            {
                //break;
            }
        }
        last_time = cur_time;
        CURSORINFO ci = { 0 };
        ci.cbSize = sizeof(CURSORINFO);
        GetCursorInfo(&ci);

        printf_s("cursor pos: (%d, %d)\n", ci.ptScreenPos.x, ci.ptScreenPos.y);

        POINT pt_new;
        GetCursorPos(&pt_new);
        printf_s("GetCursorPos: (%d, %d)\n", pt_new.x, pt_new.y);

        if (!(ci.flags & CURSOR_SHOWING))
        {
            printf_s("cursor not showed!\n");
            continue;
        }

        uint8_t* maskBits = NULL;
        uint8_t* colorBits = NULL;
        HBITMAP cursor_bmp = NULL;
        int res = CaptureCursorBitmap(ci.hCursor, maskBits, colorBits);

        if (maskBits != NULL)
        {
            delete[] maskBits;
        }

        if (colorBits != NULL)
        {
            delete[] colorBits;
        }

        if (res == 0)
        {
            cnt++;
        }
        else
        {
            printf_s("capture cursor shape icon failed! \n");
        }

        if (cnt == sample_rate)
        {
            printf_s("\ncapture %d times cursor shape, duration:%d ms\n", sample_rate, TimeMilliseconds() - start);
            start = TimeMilliseconds();
            cnt = 0;
        }
    }

    printf_s("\ncapture cursor shape icon stoped! \n");

    return 0;
}

bool CreateCaptureCursorThread()
{
    // 创建一个线程监控鼠标的形状变化，并把cursor handle存入队列中
    DWORD threadID;
    HANDLE th;
    th = NULL;
    int x = 0;
    th = CreateThread(NULL, 0, CaptureCursorBitmap, (LPVOID)pcc, 0, &threadID);
    CloseHandle(th);
    return true;
}

DWORD WINAPI DeviceListener(PVOID pvParam)
{
    WNDCLASSEXW window_class = { 0 };
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = DeviceProc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = hInst;
    window_class.hIcon = NULL;
    window_class.hCursor = NULL;
    window_class.hbrBackground = NULL;
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = L"DeviceListener";
    window_class.hIconSm = NULL;
    auto res = RegisterClassExW(&window_class);

    message_wnd_ = CreateWindowExW(
        WS_EX_LAYERED, L"DeviceListener", L"DeviceListener",
        WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInst, NULL);

    if (!message_wnd_)
    {
        printf_s("error code:%d! \n", GetLastError());
        return 0;
    }

    ShowWindow(message_wnd_, SW_HIDE);

    hwnd_next_viewer = SetClipboardViewer(message_wnd_);
    /*if (!AddClipboardFormatListener(message_wnd_))
    {
        printf_s("AddClipboardFormatListener failed! \n");
        return 0;
    }*/

    // while (1) {}

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (1)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

bool CreateDeviceListenerThread()
{
    DWORD threadID;
    HANDLE th;
    th = NULL;
    int x = 0;
    th = CreateThread(NULL, 0, DeviceListener, (LPVOID)pcc, 0, &threadID);
    CloseHandle(th);
    return true;
}

LRESULT CALLBACK DeviceProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DISPLAYCHANGE:
        printf_s("Thread screen resolution changed! new size is (%4d, %4d), virtual screen size is (%4d, %4d)\n", LOWORD(lParam), HIWORD(lParam),
                 GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
        break;
    case WM_DPICHANGED:
        printf_s("Thread DPI changed, new=%d\n", HIWORD(wParam));
        break;
    case WM_CLIPBOARDUPDATE:
    {
        break;
    }
    case WM_DRAWCLIPBOARD:
    {
        HWND owner = GetClipboardOwner();
        if (owner == message_wnd_)
        {
            printf_s("message wnd changed clipboard\n");
            break;
        }
        ProcessLocalClipboardChange();
        UpdateLocalClipboard();
        break;
    }
    case WM_CHANGECBCHAIN:
    {
        HWND hwnd_remove = (HWND)wParam;
        HWND hwnd_next = (HWND)lParam;
        if (hwnd_remove == hwnd_next_viewer)
        {
            hwnd_next_viewer = hwnd_next;
        }
        else if (hwnd_next_viewer != NULL &&
                 hwnd_next_viewer != INVALID_HANDLE_VALUE)
        {
            SendNotifyMessage(hwnd_next_viewer, WM_CHANGECBCHAIN,
                              (WPARAM)hwnd_remove, (LPARAM)hwnd_next);
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // the first argv parameter is excutable path
    // but lpCmdLine only contains parameters, so should set a fake parameter first
    std::wstring str = lpCmdLine;
    std::string args = std::string(str.begin(), str.end());
    cmdline::parser a;
    //a.add<std::string>("host", 'h', "host name", true, "");
    //a.add<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535));
    //a.add<std::string>("type", 't', "protocol type", false, "http", cmdline::oneof<std::string>("http", "https", "ssh", "ftp"));
    //a.add("gzip", '\0', "gzip when transfer");
    // --console will be treat as true
    a.add("console", '\0', "enable console");
    
    a.parse_check(args);

    bool enable_console = false;
    if (a.exist("console"))
    {
        enable_console = true;
    }

    //std::cout << a.get<std::string>("type") << "://"
    //    << a.get<std::string>("host") << ":"
    //    << a.get<int>("port") << std::endl;

    // wxh add console for print info!
#if defined(CONSOLE_OUTPUT)
    if (enable_console)
    {
        if (!AllocConsole()) {
            printf_s("open console failed!\n");
        }
        else {
            char szBuff[128];
            wsprintfA(szBuff, "debug output window, process ID: %d", GetCurrentProcessId());
            SetConsoleTitleA(szBuff);
            freopen("conin$", "r+t", stdin);
            freopen("conout$", "w+t", stdout);
            freopen("conout$", "w+t", stderr);
        }
    }
    
#endif

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); /// 设置窗口的dpi感知，否则设置的大小不正确

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VNCVIEWER, szWindowClass, MAX_LOADSTRING);
    auto res = MyRegisterClass(hInstance);
    
    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    pcc = new ClientControlCaptureImpl();

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VNCVIEWER));
    
    // 主消息循环:
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VNCVIEWER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_VNCVIEWER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        500, 100, 600, 1200, nullptr, nullptr, hInstance, nullptr);
   if (!hWnd)
   {
      return FALSE;
   }
   CreateChildWindow();

   
   UINT m_Dpi = GetDeviceCaps(GetDC(hWnd), LOGPIXELSX);
   RECT rt;
   GetWindowRect(hWnd, &rt);
   AdjustWindowRectExForDpi(&rt, GetWindowLong(hWnd, GWL_STYLE) & ~WS_VSCROLL & ~WS_HSCROLL,
                            FALSE,
                            GetWindowLong(hWnd, GWL_EXSTYLE),
                            m_Dpi);
   
   SetWindowPos(hWnd, NULL, 0, 0, rt.right - rt.left, rt.bottom - rt.top, SWP_NOMOVE);
   printf_s("width %d, height %d dpiwindow %d\n", rt.right - rt.left, rt.bottom - rt.top, m_Dpi);

   ShowWindow(hWnd, nCmdShow);
   ShowWindow(hWndChild, nCmdShow);
   MoveWindow(hWndChild, 30, 100, 540, 960, true);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            break;
        }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_STARTCAPTURE:
                // init hook
                printf_s("\n######start all hooks! \n \n");
                pcc->SetCaptureWindow(hWnd);
                if (pcc->InstallMouseHook())
                {
                    printf_s("install mouse hook success!\n");
                }
                if (pcc->InstallKeyBoardHook())
                {
                    printf_s("install keyboard hook success!\n");
                }
                break;
            case IDM_STOPCAPTURE:
                printf_s("\n######stop all hooks! \n \n");
                pcc->UnintsallMouseHook();
                pcc->UninstallKeyBoardHook();
                break;
            case IDM_STARTCURSOR:
                CreateCaptureCursorThread();
                break;
            case ID_STOPCURSOR:
                captureCursor = false;
                break;
            case IDM_SHOWKEYBOARD:
                ShowKeyboard();
                break;
            case IDM_SHOWMOUSE:
                ShowPointer();
                break;
            case IDM_SHOWCURSOR:
                ShowCursor();
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_DISPLAYCHANGE:
        printf_s("screen resolution changed! new size is (%4d, %4d), virtual screen size is (%4d, %4d)\n", LOWORD(lParam), HIWORD(lParam),
                 GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
        break;
    case WM_DPICHANGED:
        printf_s("DPI changed, new=%d\n", HIWORD(wParam));
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndChildProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 分析菜单选择:
        switch (wmId)
        {
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: 在此处添加使用 hdc 的任何绘图代码...
        HDC temp = GetWindowDC(hWnd);
        HDC bmp1;
        bmp1 = CreateCompatibleDC(NULL);
        SelectObject(bmp1, bmp);
        BLENDFUNCTION ftn = { 0 };
        ftn.BlendOp = AC_SRC_OVER;
        ftn.SourceConstantAlpha = 255;
        ftn.AlphaFormat = AC_SRC_ALPHA;
        AlphaBlend(temp, 50, 50, 32, 32, bmp1, 0, 0, 32, 32, ftn);
        // BitBlt(temp, 50, 50, 32, 32, bmp1, 0, 0, SRCCOPY);
        ReleaseDC(hWnd, temp);
        DeleteDC(bmp1);

        EndPaint(hWnd, &ps);
    }
    break;
    case WM_SETCURSOR:
    {
        // printf_s("set global cursor triggered, wparam:%d, lParam:%ld\n", wParam, lParam);
        DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


void ShowKeyboard()
{
    Sleep(2000);
    vncKeymap::ClearShiftKeys();
    while (!keyQ.empty()) 
    {
        rfbKeyEventMsg msg = keyQ.front();
        keyQ.pop();
        vncKeymap::KeyEvent(msg.key, (0 != msg.down), false, false);
        Sleep(5);
    }
}

void ShowPointer()
{
    Sleep(3000);
    PointerEventProc pointer;
    pointer.Init();

    // 选择一个显示器回放鼠标消息，0表示主显示器
    pointer.ChooseMonitor(1);

    while (!pointerQ.empty()) 
    {
        rfbPointerEventMsg pe = pointerQ.front();
        pointerQ.pop();
        pointer.ExcutePointerEvent(pe);
        Sleep(10);
    }
}

void ShowCursor()
{
    Sleep(3000);
    
    printf_s("\n####### start show cursor events \n");

    HCURSOR h;
    while (!hcursorQ.empty())
    {
        h = hcursorQ.front();
        hcursorQ.pop();
        int res = SetClassLongPtr(hWndChild, GCLP_HCURSOR, (LONG_PTR)(h)); // 当前代码与hWndChild不在同一个线程时生效
        printf_s("SetClassLongPtr status %d!\n", res);
        SetCursor(h);                                                  // 当前代码与hWndChild在同一个线程时生效
        Sleep(1000);
    }

    printf_s("####### end show cursor events \n");
}

void CreateChildWindow()
{
    HCURSOR h1;
    h1 = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    // h1 = LoadCursorFromFile(_T("cursor1.cur"));

    WNDCLASS wcex;

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndChildProc;
    wcex.cbClsExtra = sizeof(long);
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hIcon = NULL;
    wcex.hCursor = h1;
    wcex.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = TEXT("ChildWindow");
    RegisterClass(&wcex);

    hWndChild = CreateWindowW(TEXT("ChildWindow"), NULL, WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100, hWnd, NULL, hInst, nullptr);
    
    if (!hWndChild)
    {
        printf_s("create child Window failed, error code %d!\n", GetLastError());
    }
}
