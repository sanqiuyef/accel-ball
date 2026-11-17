#include "common.h"
#include "ball_window.h"

#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\AccelBall.SingleInstance");
    if (mutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return 0;
    }

    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) {
        if (mutex != nullptr) {
            CloseHandle(mutex);
        }
        return 1;
    }

    int result = 0;
    {
        BallWindow window;
        if (window.Create(instance)) {
            window.Run();
        } else {
            result = 1;
        }
    }

    Gdiplus::GdiplusShutdown(token);
    if (mutex != nullptr) {
        CloseHandle(mutex);
    }
    return result;
}
