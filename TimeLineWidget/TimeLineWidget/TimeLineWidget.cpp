#include <windows.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <wchar.h>
#include "resource2.h"
#include <mmsystem.h> // Added for PlaySound
#pragma comment(lib, "Winmm.lib") // Link Winmm.lib (for MSVC)
using namespace Gdiplus;

// 全局变量
HWND hWnd;
WCHAR taskName[256] = L"Default Task"; // 任务名称
int taskDuration = 10000; // 任务时间（毫秒），默认10秒
int elapsedTime = 0;
bool isRunning = true;
int modeSelection = 0; // 0: 进度条渐短，1：进度条渐长
int currentWidth = 0; // 计算当前进度条宽度
static HDC hdcMem = NULL;        // 内存 DC
static HBITMAP hBitmap = NULL;   // 兼容位图
static HBITMAP hOld = NULL;      // 保存原始位图

// 托盘图标相关
#define ID_TRAYICON 1
NOTIFYICONDATA nid = { 0 };

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 初始化 GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 解析命令行参数
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 4)
    {
        wcsncpy_s(taskName, argv[1], 255);
        taskDuration = _wtoi(argv[2]) * 1000; // 秒转毫秒
        modeSelection = _wtoi(argv[3]);
    }
    LocalFree(argv);

    // 注册窗口类
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TimeLineWidget";
    wc.hbrBackground = NULL; // 不用默认背景
    RegisterClass(&wc);

    // 获取屏幕宽度
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);

    // 创建分层窗口（透明窗口）
    hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, // 添加 WS_EX_TOOLWINDOW
        L"TimeLineWidget", L"TimeLine Widget",
        WS_POPUP, // 移除 WS_VISIBLE
        0, 0, screenWidth, 4,
        NULL, NULL, hInstance, NULL);

    // 手动显示窗口（非激活方式）
    ShowWindow(hWnd, SW_SHOWNA);
    UpdateWindow(hWnd);

    // 初始化托盘图标
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // 使用资源中的图标
    wcscpy_s(nid.szTip, L"TimeLine Widget");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理托盘图标
    Shell_NotifyIcon(NIM_DELETE, &nid);
    GdiplusShutdown(gdiplusToken);
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // 设置定时器
        SetTimer(hWnd, 1, 16, NULL); // 每 16ms 更新一次

        // 创建内存 DC 和兼容位图
        HDC hdcScreen = GetDC(NULL);
        hdcMem = CreateCompatibleDC(hdcScreen);
        hBitmap = CreateCompatibleBitmap(hdcScreen, GetSystemMetrics(SM_CXSCREEN), 4);
        hOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
        ReleaseDC(NULL, hdcScreen);

        // 错误检查
        if (!hdcMem || !hBitmap)
        {
            PostQuitMessage(1);
            return -1;
        }
        break;
    }

    case WM_TIMER:
    {
        static ULONGLONG startTime = GetTickCount64();
        ULONGLONG now = GetTickCount64();
        elapsedTime = static_cast<int>(now - startTime);

        if (elapsedTime >= taskDuration) {
            KillTimer(hWnd, 1);

            // 计算小时和分钟
            int totalSeconds = taskDuration / 1000; // 转换为秒
            int hours = totalSeconds / 3600; // 小时
            int minutes = (totalSeconds % 3600) / 60; // 分钟

            // 格式化通知消息
            WCHAR szInfo[256];
            swprintf_s(szInfo, 256, L"任务已完成！\n任务时长：%d小时%d分钟", hours, minutes);

            // 显示任务完成通知
            nid.uFlags = NIF_INFO;
            wcscpy_s(nid.szInfo, szInfo);
            wcscpy_s(nid.szInfoTitle, taskName);
            nid.dwInfoFlags = NIIF_INFO;
            Shell_NotifyIcon(NIM_MODIFY, &nid);

            // 播放提示音
            PlaySoundW(L"notification.wav", NULL, SND_FILENAME | SND_ASYNC);
        }

        // 获取屏幕宽度
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);

        // 计算宽度
        if (modeSelection) {
            currentWidth = static_cast<int>(screenWidth * (static_cast<float>(elapsedTime) / taskDuration));
        }
        else {
            currentWidth = static_cast<int>(screenWidth * (static_cast<float>(taskDuration - elapsedTime) / taskDuration));
        }

        // 绘制进度条
        {
            Graphics g(hdcMem);
            g.SetSmoothingMode(SmoothingModeNone);
            g.Clear(Color(0, 0, 0, 0)); // 完全透明背景

            if (currentWidth > 0) {
                LinearGradientBrush brush(
                    Point(0, 0), Point(currentWidth, 0),
                    Color(255, 0, 120, 255),     // 紫色
                    Color(255, 255, 165, 255));  // 粉色
                g.FillRectangle(&brush, 0, 0, currentWidth, 4);
            }
        }

        // 更新分层窗口
        HDC hdcScreen = GetDC(NULL);
        POINT ptSrc = { 0, 0 };
        SIZE sizeWnd = { screenWidth, 4 };
        POINT ptDst = { 0, 0 };

        BLENDFUNCTION blend = { 0 };
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
        ReleaseDC(NULL, hdcScreen);
        break;
    }

    case WM_USER + 1: // 托盘通知回调
        switch (lParam)
        {
        case WM_RBUTTONUP: // 右键点击托盘图标
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1, L"退出");
            SetForegroundWindow(hWnd); // 确保菜单正常显示
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
            break;
        }
        case NIN_BALLOONHIDE:       // 用户点X关闭
        case NIN_BALLOONTIMEOUT:    // 自动消失
        case NIN_BALLOONUSERCLICK:  // 用户点击通知
            // 停止播放声音
            PlaySound(NULL, NULL, 0);
            // 退出程序
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) // 退出菜单项
        {
            PlaySound(NULL, NULL, 0);
            PostQuitMessage(0);
        }
        break;

    case WM_DESTROY:
    {
        // 清理内存 DC 和位图
        if (hdcMem && hBitmap) {
            SelectObject(hdcMem, hOld);
            DeleteObject(hBitmap);
            DeleteDC(hdcMem);
            hdcMem = NULL;
            hBitmap = NULL;
            hOld = NULL;
        }
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
