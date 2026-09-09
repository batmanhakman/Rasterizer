#ifndef UNICODE
#define UNICODE
#endif

#include <iostream>
#include <window.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register the window
    const wchar_t CLASS_NAME[] = L"DefaultProgramName";

    WNDCLASS wc = { };

    // window procedure defines most of the behavior of the window. For now, this value is a forward declaration of a function. 
    wc.lpfnWndProc = WindowProc;
    //  handles to the application instance. Get this value from the hInstance parameter of wWinMain.
    wc.hInstance = hInstance;
    // a string that identifies the window class
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx
    (
        0,
        CLASS_NAME, // WindowClass
        L"RasterizerWinodw",// Window Text
        WS_OVERLAPPEDWINDOW, //Window Style

        CW_USEDEFAULT, // X position
        CW_USEDEFAULT, // Y position
        1920, // Width
        1080, // Height
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    return hwnd;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        switch (uMsg)
    {
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
        }
        break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // all painting occurs here

            FillRect(hdc, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));

            EndPaint(hwnd, &ps);
        }
        return 0;

        case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

        case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
    
