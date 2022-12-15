// vncviewer.cpp : 定义应用程序的入口点。
//
#define _CRT_SECURE_NO_WARNINGS
#include "framework.h"
#include "ClientConnection.h"
#include "vncviewer.h"
#include <iostream>
#include <queue>
#include "vncKeymap.h"
#include "pointerProc.h"
#include <Windows.h>
#include <utility>
#include "bitmap.h"
#include <wingdi.h>

#pragma comment(lib, "Msimg32.lib") // gdi
#define MAX_LOADSTRING 100



// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

DWORD thdID = 0;

bool captureCursor = true;
HWND hWnd = nullptr;
HWND hWndChild = nullptr;
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
LRESULT CALLBACK    WndChildProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void ShowKeyboard();
void ShowPointer();
void ShowCursor();
void CreateChildWindow();

DWORD WINAPI capture_cursor(PVOID pvParam)
{
    printf_s("\ncapture cursor shape started! \n");
    HCURSOR lastC = nullptr;
    while (captureCursor) 
    {
        CURSORINFO ci = { 0 };
        ci.cbSize = sizeof(CURSORINFO);
        if (GetCursorInfo(&ci) && lastC != ci.hCursor)
        {
            lastC = ci.hCursor;
            printf_s("cursor shape update, cursor:%p\n", lastC);
            hcursorQ.push(lastC);
        }
    }
    printf_s("\ncapture cursor shape stoped! \n");

    
    // 工作线程设置鼠标形状，并向ui窗口发消息
    while (!hcursorQ.empty())
    {
        g_h1 = hcursorQ.front();
        hcursorQ.pop();
        int res = SetClassLongPtr(hWndChild, GCLP_HCURSOR, (LONG_PTR)(g_h1));
        PostMessage(hWndChild, WM_SETCURSOR, (WPARAM)(hWndChild), 33554433);
        printf_s("SetClassLongPtr res status:%d\n", res);
        Sleep(1000);
    }

    return 0;
}

DWORD WINAPI capture_cursor_icon(PVOID pvParam)
{
    Sleep(1000);
    printf_s("\ncapture cursor shape icon started! \n");
    
    while (captureCursor)
    {
        CURSORINFO ci = { 0 };
        ci.cbSize = sizeof(CURSORINFO);
        GetCursorInfo(&ci);

        if (!(ci.flags & CURSOR_SHOWING))
        {
            printf_s("cursor not showed!\n");
            continue;
        }

        HICON hIcon = CopyIcon(ci.hCursor);
        if (!hIcon)
        {
            printf_s("copyIcon failed!\n");
            continue;
        }

        // bmp = IconToBitmap(hIcon);
        // SendMessage(hWndChild, WM_PAINT, NULL, NULL);
        // cursorToBitmap(ci);

        uint8_t* maskBits = NULL;
        uint8_t* colorBits = NULL;
        HBITMAP cursor_bmp = NULL;
        int res = captureCursorIcon(hIcon, maskBits, colorBits, bmp);
        SendMessage(hWndChild, WM_PAINT, NULL, NULL);

        if (maskBits != NULL)
        {
            delete[] maskBits;
        }

        if (colorBits != NULL)
        {
            delete[] colorBits;
        }

        DestroyIcon(hIcon);
        Sleep(10);
    }

    printf_s("\ncapture cursor shape icon stoped! \n");

    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


    // wxh add console for print info!
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
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); /// 设置窗口的dpi感知，否则设置的大小不正确
    
    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VNCVIEWER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    pcc = new ClientControlCaptureImpl();

    // 创建一个线程监控鼠标的形状变化，并把cursor handle存入队列中
    DWORD threadID;
    HANDLE th;
    th = NULL;
    int x = 0;
    th = CreateThread(NULL, 0, capture_cursor_icon, (LPVOID)pcc, 0, &threadID);
    CloseHandle(th);

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



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
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

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
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

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
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
                captureCursor = false;
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
                captureCursor = false;
                pcc->UnintsallMouseHook();
                pcc->UninstallKeyBoardHook();
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

// “关于”框的消息处理程序。
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
    h1 = CreateCursor(NULL, 0, 0, 32, 32, and_mask_bits, xor_mask_bits);
    h1 = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    // h1 = LoadCursorFromFile(_T("cursor1.cur"));

    WNDCLASS wcex;

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndChildProc;
    wcex.cbClsExtra = sizeof(long);;
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
