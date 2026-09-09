#pragma once
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#include <windows.h>

HWND InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int mCmdShow);