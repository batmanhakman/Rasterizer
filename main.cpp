#ifdef _WIN32
#include "window.h"
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    return RunWindowsApplication(instance, showCommand);
}
#else
#include "macos.h"
int main() { return RunApplication(); }
#endif
