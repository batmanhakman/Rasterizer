#include "Rasterizer.h"

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif

#include <filesystem>

#include "window.h"
#include "ObjLoader.h"

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

    wchar_t executablePath[MAX_PATH];
    const DWORD executablePathLength =
        GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    if (executablePathLength == 0 || executablePathLength == MAX_PATH)
    {
        MessageBox(
            hwnd,
            L"Could not determine the executable directory.",
            L"Rasterizer",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    const std::filesystem::path objPath =
        std::filesystem::path(
            std::wstring(executablePath, executablePathLength))
        .parent_path() / L"armadillo.obj";
    const std::string objFile = objPath.string();

    Mesh armadilloMesh;
    if (!ObjLoader::LoadVertices(objFile, armadilloMesh) ||
        !ObjLoader::LoadFaces(objFile, armadilloMesh))
    {
        MessageBox(
            hwnd,
            L"Could not load armadillo.obj.",
            L"Rasterizer",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    // Match the macOS scene: place the imported model at the pyramid depth.
    for (Vertex& vertex : armadilloMesh.vertices)
    {
        vertex.z += 450.0f;
    }

    Camera camera = {
        Vector3D{0.0f, 0.0f, 0.0f},
        0.0f,
        0.0f,
        500.0f};
    const uint32_t armadilloColor = fb.color(255, 128, 0, 255);
    Rasterizer::DrawMesh(fb, armadilloMesh, camera, armadilloColor);

    ShowWindow(hwnd, nCmdShow);
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
#else
#include "macos.h"

int main()
{
    return RunApplication();
}
#endif
