#ifndef UNICODE
#define UNICODE
#endif

#include <iostream>
#include "window.h"
#include "framebuffer.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Updated signature to take pFramebuffer
HWND InitWindow(HINSTANCE hInstance, int nCmdShow, Framebuffer* pFramebuffer)
{
    // Register the window class
    const wchar_t CLASS_NAME[] = L"DefaultProgramName";

    WNDCLASS wc = { };

    // window procedure defines most of the behavior of the window.
    wc.lpfnWndProc = WindowProc;
    // handles to the application instance. Get this value from the hInstance parameter of wWinMain.
    wc.hInstance = hInstance;
    // a string that identifies the window class
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx
    (
        0,
        CLASS_NAME,          // WindowClass
        L"RasterizerWindow", // Window Text
        WS_OVERLAPPEDWINDOW, // Window Style

        CW_USEDEFAULT,       // X position
        CW_USEDEFAULT,       // Y position
        1920,                // Width
        1080,                // Height
        NULL, 
        NULL, 
        hInstance, 
        pFramebuffer         // Passed pointer as lpParam 
    );

    ShowWindow(hwnd, nCmdShow);
    return hwnd;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve the pointer to framebuffer stored on the window handle
    Framebuffer* pFb = reinterpret_cast<Framebuffer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
        case WM_CREATE:
        {
            // Get the Framebuffer pointer passed from CreateWindowEx
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pFb = reinterpret_cast<Framebuffer*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pFb));
            return 0;
        }

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

            // Copy Framebuffer pixels onto screen
            if (pFb != nullptr)
            {
                BITMAPINFO bmi = {};
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = pFb->GetWidth();
                bmi.bmiHeader.biHeight = -pFb->GetHeight(); // Negative height for top-down orientation
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                StretchDIBits(
                    hdc,
                    0, 0, pFb->GetWidth(), pFb->GetHeight(),
                    0, 0, pFb->GetWidth(), pFb->GetHeight(),
                    pFb->GetBuffer(),
                    &bmi,
                    DIB_RGB_COLORS,
                    SRCCOPY
                );
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}