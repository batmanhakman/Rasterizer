#ifndef UNICODE
#define UNICODE
#endif

#include "window.h"
#include "Rasterizer.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    HWND hwnd = InitWindow(hInstance, nCmdShow);
    if (hwnd == NULL)
    {
        return 0;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    Framebuffer fb(width, height);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&fb));

    uint32_t color = 0x00FF0000;
    Rasterizer::LineDraw(fb, 100, 100, 500, 300, color);

    InvalidateRect(hwnd, NULL, TRUE);

    // Run Message loop
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        // related to keyboard input ALWAYS CALL IT BEFORE DISPATCH MESSAGE.
        TranslateMessage(&msg);
        // Invokes function based on what the function pointer points at.
        DispatchMessage(&msg);
    }

    return (int)msg.wParam; //Returns int
}
