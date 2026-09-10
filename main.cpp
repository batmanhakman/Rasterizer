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


    Framebuffer::Framebuffer(int width, int height);



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
