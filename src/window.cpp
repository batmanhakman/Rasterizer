#include "window.h"
#include "Scene.h"
#include <windowsx.h>
#include <chrono>
#include <exception>
#include <string>

namespace
{
struct Application
{
    Scene scene;
    SceneInput input;
    bool dragging = false;
    POINT previousMouse{};
    std::chrono::steady_clock::time_point previousTick = std::chrono::steady_clock::now();
};

std::wstring Wide(const std::string& text)
{
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size);
    return result;
}

void ShowError(HWND window, const std::string& message)
{
    MessageBoxW(window, Wide(message).c_str(), L"Rasterizer - unable to continue", MB_OK | MB_ICONERROR);
}

void SetKey(SceneInput& input, WPARAM key, bool pressed)
{
    switch (key)
    {
    case VK_LEFT: case 'A': input.left = pressed; break;
    case VK_RIGHT: case 'D': input.right = pressed; break;
    case VK_UP: input.up = pressed; break;
    case VK_DOWN: input.down = pressed; break;
    case 'W': input.zoomIn = pressed; break;
    case 'S': input.zoomOut = pressed; break;
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        app = static_cast<Application*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (!app) return DefWindowProcW(window, message, wParam, lParam);
    try
    {
        switch (message)
        {
        case WM_CREATE:
            if (!SetTimer(window, 1, 16, nullptr)) return -1;
            return 0;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                app->scene.Resize(LOWORD(lParam), HIWORD(lParam));
                app->scene.Render();
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;
        case WM_TIMER:
        {
            const auto now = std::chrono::steady_clock::now();
            const float elapsed = std::chrono::duration<float>(now - app->previousTick).count();
            app->previousTick = now;
            if (!IsIconic(window))
            {
                app->scene.Update(app->input, elapsed);
                app->scene.Render();
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;
        }
        case WM_KEYDOWN:
            SetKey(app->input, wParam, true);
            if (wParam == 'R') app->scene.Reset();
            if (wParam == VK_ESCAPE) DestroyWindow(window);
            return 0;
        case WM_KEYUP:
            SetKey(app->input, wParam, false);
            return 0;
        case WM_KILLFOCUS:
            app->input = {};
            app->dragging = false;
            if (GetCapture() == window) ReleaseCapture();
            return 0;
        case WM_LBUTTONDOWN:
            SetFocus(window);
            app->dragging = true;
            app->previousMouse = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            SetCapture(window);
            return 0;
        case WM_MOUSEMOVE:
            if (app->dragging)
            {
                POINT mouse{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                app->scene.Orbit((mouse.x - app->previousMouse.x) * 0.006f,
                                 (mouse.y - app->previousMouse.y) * 0.006f);
                app->previousMouse = mouse;
            }
            return 0;
        case WM_LBUTTONUP:
            app->dragging = false;
            if (GetCapture() == window) ReleaseCapture();
            return 0;
        case WM_CAPTURECHANGED:
            app->dragging = false;
            return 0;
        case WM_MOUSEWHEEL:
            app->scene.Zoom(static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT paint{};
            const HDC dc = BeginPaint(window, &paint);
            const Framebuffer& frame = app->scene.Frame();
            BITMAPINFO bitmap{};
            bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmap.bmiHeader.biWidth = frame.GetWidth();
            bitmap.bmiHeader.biHeight = -frame.GetHeight();
            bitmap.bmiHeader.biPlanes = 1;
            bitmap.bmiHeader.biBitCount = 32;
            bitmap.bmiHeader.biCompression = BI_RGB;
            RECT client{};
            GetClientRect(window, &client);
            StretchDIBits(dc, 0, 0, client.right, client.bottom,
                0, 0, frame.GetWidth(), frame.GetHeight(), frame.GetBuffer(),
                &bitmap, DIB_RGB_COLORS, SRCCOPY);
            EndPaint(window, &paint);
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            KillTimer(window, 1);
            PostQuitMessage(0);
            return 0;
        }
    }
    catch (const std::exception& error)
    {
        KillTimer(window, 1);
        ShowError(window, error.what());
        DestroyWindow(window);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
}

int RunWindowsApplication(HINSTANCE instance, int showCommand)
{
    try
    {
        SetProcessDPIAware();
        Application app;
        std::string error;
        if (!app.scene.Load(Scene::DefaultModelPath(), error))
        {
            ShowError(nullptr, "Could not load the knight. Keep the assets folder beside Rasterizer.exe.\n\n" + error);
            return 1;
        }
        app.scene.Render();
        const wchar_t* className = L"RasterizerKnightWindow";
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = className;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        if (!RegisterClassW(&windowClass))
        {
            ShowError(nullptr, "Could not register the native window.");
            return 1;
        }
        RECT bounds{0, 0, 1000, 760};
        AdjustWindowRect(&bounds, WS_OVERLAPPEDWINDOW, FALSE);
        HWND window = CreateWindowExW(0, className,
            L"Rasterizer | Knight - drag / arrows: orbit  W/S / wheel: zoom  R: reset  Esc: quit",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            bounds.right - bounds.left, bounds.bottom - bounds.top,
            nullptr, nullptr, instance, &app);
        if (!window)
        {
            ShowError(nullptr, "Could not create the native window.");
            return 1;
        }
        ShowWindow(window, showCommand);
        UpdateWindow(window);
        MSG message{};
        BOOL result;
        while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0)
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        return result == -1 ? 1 : static_cast<int>(message.wParam);
    }
    catch (const std::exception& error)
    {
        ShowError(nullptr, error.what());
        return 1;
    }
}
