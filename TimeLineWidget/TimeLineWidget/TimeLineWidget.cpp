#include <windows.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <wchar.h>
#include "resource2.h"
#include <mmsystem.h> // Added for PlaySound
#pragma comment(lib, "Winmm.lib") // Link Winmm.lib (for MSVC)
using namespace Gdiplus;

// ȫ�ֱ���
HWND hWnd;
WCHAR taskName[256] = L"Default Task"; // ��������
int taskDuration = 10000; // ����ʱ�䣨���룩��Ĭ��10��
int elapsedTime = 0;
bool isRunning = true;
int modeSelection = 0; // 0: ���������̣�1������������
int currentWidth = 0; // ���㵱ǰ���������
static HDC hdcMem = NULL;        // �ڴ� DC
static HBITMAP hBitmap = NULL;   // ����λͼ
static HBITMAP hOld = NULL;      // ����ԭʼλͼ

// ����ͼ�����
#define ID_TRAYICON 1
NOTIFYICONDATA nid = { 0 };

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // ��ʼ�� GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // ���������в���
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 4)
    {
        wcsncpy_s(taskName, argv[1], 255);
        taskDuration = _wtoi(argv[2]) * 1000; // ��ת����
        modeSelection = _wtoi(argv[3]);
    }
    LocalFree(argv);

    // ע�ᴰ����
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TimeLineWidget";
    wc.hbrBackground = NULL; // ����Ĭ�ϱ���
    RegisterClass(&wc);

    // ��ȡ��Ļ���
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);

    // �����ֲ㴰�ڣ�͸�����ڣ�
    hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, // ��� WS_EX_TOOLWINDOW
        L"TimeLineWidget", L"TimeLine Widget",
        WS_POPUP, // �Ƴ� WS_VISIBLE
        0, 0, screenWidth, 4,
        NULL, NULL, hInstance, NULL);

    // �ֶ���ʾ���ڣ��Ǽ��ʽ��
    ShowWindow(hWnd, SW_SHOWNA);
    UpdateWindow(hWnd);

    // ��ʼ������ͼ��
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // ʹ����Դ�е�ͼ��
    wcscpy_s(nid.szTip, L"TimeLine Widget");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // ��Ϣѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // ��������ͼ��
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
        // ���ö�ʱ��
        SetTimer(hWnd, 1, 16, NULL); // ÿ 16ms ����һ��

        // �����ڴ� DC �ͼ���λͼ
        HDC hdcScreen = GetDC(NULL);
        hdcMem = CreateCompatibleDC(hdcScreen);
        hBitmap = CreateCompatibleBitmap(hdcScreen, GetSystemMetrics(SM_CXSCREEN), 4);
        hOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
        ReleaseDC(NULL, hdcScreen);

        // ������
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

            // ����Сʱ�ͷ���
            int totalSeconds = taskDuration / 1000; // ת��Ϊ��
            int hours = totalSeconds / 3600; // Сʱ
            int minutes = (totalSeconds % 3600) / 60; // ����

            // ��ʽ��֪ͨ��Ϣ
            WCHAR szInfo[256];
            swprintf_s(szInfo, 256, L"��������ɣ�\n����ʱ����%dСʱ%d����", hours, minutes);

            // ��ʾ�������֪ͨ
            nid.uFlags = NIF_INFO;
            wcscpy_s(nid.szInfo, szInfo);
            wcscpy_s(nid.szInfoTitle, taskName);
            nid.dwInfoFlags = NIIF_INFO;
            Shell_NotifyIcon(NIM_MODIFY, &nid);

            // ������ʾ��
            PlaySoundW(L"notification.wav", NULL, SND_FILENAME | SND_ASYNC);
        }

        // ��ȡ��Ļ���
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);

        // ������
        if (modeSelection) {
            currentWidth = static_cast<int>(screenWidth * (static_cast<float>(elapsedTime) / taskDuration));
        }
        else {
            currentWidth = static_cast<int>(screenWidth * (static_cast<float>(taskDuration - elapsedTime) / taskDuration));
        }

        // ���ƽ�����
        {
            Graphics g(hdcMem);
            g.SetSmoothingMode(SmoothingModeNone);
            g.Clear(Color(0, 0, 0, 0)); // ��ȫ͸������

            if (currentWidth > 0) {
                LinearGradientBrush brush(
                    Point(0, 0), Point(currentWidth, 0),
                    Color(255, 0, 120, 255),     // ��ɫ
                    Color(255, 255, 165, 255));  // ��ɫ
                g.FillRectangle(&brush, 0, 0, currentWidth, 4);
            }
        }

        // ���·ֲ㴰��
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

    case WM_USER + 1: // ����֪ͨ�ص�
        switch (lParam)
        {
        case WM_RBUTTONUP: // �Ҽ��������ͼ��
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1, L"�˳�");
            SetForegroundWindow(hWnd); // ȷ���˵�������ʾ
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
            break;
        }
        case NIN_BALLOONHIDE:       // �û���X�ر�
        case NIN_BALLOONTIMEOUT:    // �Զ���ʧ
        case NIN_BALLOONUSERCLICK:  // �û����֪ͨ
            // ֹͣ��������
            PlaySound(NULL, NULL, 0);
            // �˳�����
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) // �˳��˵���
        {
            PlaySound(NULL, NULL, 0);
            PostQuitMessage(0);
        }
        break;

    case WM_DESTROY:
    {
        // �����ڴ� DC ��λͼ
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
