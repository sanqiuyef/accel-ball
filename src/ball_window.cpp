#include "ball_window.h"

#include "cleaner.h"
#include "resource.h"
#include "util.h"

#include <algorithm>
#include <cstdlib>
#include <cwchar>
#include <thread>

namespace {

constexpr wchar_t kWindowClass[] = L"AccelBallWindow";
constexpr wchar_t kWindowTitle[] = L"加速球";

constexpr int kBallSizeDip = 74;
constexpr int kBarLongDip = kBallSizeDip;
constexpr int kBarShortDip = 30;
constexpr int kDockThresholdDip = 22;
constexpr int kDefaultMarginDip = 24;

constexpr UINT_PTR kTimerMetrics = 1;
constexpr UINT_PTR kTimerCollapse = 2;
constexpr UINT kMessageCleanDone = WM_APP + 1;
constexpr UINT kCommandClean = 40001;
constexpr UINT kCommandExit = 40002;

int ReadIniInt(const std::wstring& path, const wchar_t* section, const wchar_t* key, int fallback) {
    wchar_t buffer[64]{};
    GetPrivateProfileStringW(section, key, L"", buffer, 64, path.c_str());
    if (buffer[0] == L'\0') {
        return fallback;
    }
    return static_cast<int>(wcstol(buffer, nullptr, 10));
}

}  // namespace

bool BallWindow::Create(HINSTANCE instance) {
    instance_ = instance;

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_DBLCLKS;
    windowClass.lpfnWndProc = &BallWindow::WndProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APPICON));
    windowClass.hIconSm = windowClass.hIcon;
    windowClass.lpszClassName = kWindowClass;

    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    hwnd_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, kWindowClass,
                            kWindowTitle, WS_POPUP, 0, 0, Scale(kBallSizeDip),
                            Scale(kBallSizeDip), nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr) {
        return false;
    }

    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    return true;
}

void BallWindow::Run() {
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

LRESULT CALLBACK BallWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    BallWindow* self = reinterpret_cast<BallWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<BallWindow*>(create->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    if (self != nullptr) {
        return self->HandleMessage(message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT BallWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_TIMER:
            OnTimer(wParam);
            return 0;
        case WM_MOUSEMOVE:
            OnMouseMove();
            return 0;
        case WM_LBUTTONDOWN:
            OnLButtonDown();
            return 0;
        case WM_LBUTTONUP:
            OnLButtonUp();
            return 0;
        case WM_LBUTTONDBLCLK:
            OnLButtonDblClk(POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            return 0;
        case WM_RBUTTONUP: {
            POINT point{};
            GetCursorPos(&point);
            ShowMenu(point);
            return 0;
        }
        case WM_MOUSELEAVE:
            OnMouseLeave();
            return 0;
        case WM_DISPLAYCHANGE:
            ApplyModeLayout(true);
            return 0;
        case WM_SETTINGCHANGE:
            if (wParam == SPI_SETWORKAREA) {
                ApplyModeLayout(true);
            }
            return 0;
        case WM_DPICHANGED:
            OnDpiChanged(HIWORD(wParam));
            return 0;
        case kMessageCleanDone:
            OnCleanDone(static_cast<unsigned long long>(wParam));
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            OnDestroy();
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd_, message, wParam, lParam);
    }
}

void BallWindow::OnCreate() {
    dpi_ = GetDpiForWindow(hwnd_);
    if (dpi_ == 0) {
        dpi_ = 96;
    }

#ifdef ACCELBALL_DEV
    // 开发版自测用：允许用环境变量强制 DPI，便于在 100% 屏幕上检查 125%/150% 布局。
    wchar_t forcedDpi[16]{};
    if (GetEnvironmentVariableW(L"ACCELBALL_DPI", forcedDpi, 16) > 0) {
        const int value = static_cast<int>(wcstol(forcedDpi, nullptr, 10));
        if (value >= 96 && value <= 480) {
            dpi_ = static_cast<UINT>(value);
        }
    }
#endif

    LoadConfig();
    UpdateMetrics();
    SetTimer(hwnd_, kTimerMetrics, 1000, nullptr);
    ApplyModeLayout(false);
    Paint();
}

void BallWindow::OnDestroy() {
    SaveConfig();
    KillTimer(hwnd_, kTimerMetrics);
    KillTimer(hwnd_, kTimerCollapse);
    // 不要清空 hwnd_：随后还有 WM_NCDESTROY 需要交给 DefWindowProc。
}

void BallWindow::OnTimer(UINT_PTR timerId) {
    if (timerId == kTimerMetrics) {
        UpdateMetrics();
        Paint();
        return;
    }

    if (timerId == kTimerCollapse) {
        KillTimer(hwnd_, kTimerCollapse);
        if (dragging_ || !hoverExpanded_ || mode_ != ViewMode::Bar) {
            return;
        }

        POINT cursor{};
        GetCursorPos(&cursor);
        RECT rect{};
        GetWindowRect(hwnd_, &rect);
        InflateRect(&rect, Scale(10), Scale(10));
        if (!PtInRect(&rect, cursor)) {
            hoverExpanded_ = false;
            ApplyModeLayout(true);
        }
    }
}

void BallWindow::OnMouseMove() {
    if (dragging_) {
        POINT cursor{};
        GetCursorPos(&cursor);
        const int deltaX = cursor.x - dragCursorStart_.x;
        const int deltaY = cursor.y - dragCursorStart_.y;
        if (std::abs(deltaX) > 2 || std::abs(deltaY) > 2) {
            dragMoved_ = true;
        }
        SetWindowPos(hwnd_, HWND_TOPMOST, dragWindowStart_.left + deltaX,
                     dragWindowStart_.top + deltaY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        return;
    }

    if (mode_ != ViewMode::Bar) {
        return;
    }

    if (!hoverExpanded_) {
        hoverExpanded_ = true;
        ApplyModeLayout(true);
    }

    KillTimer(hwnd_, kTimerCollapse);
    TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd_, 0};
    TrackMouseEvent(&track);
}

void BallWindow::OnLButtonDown() {
    if (dragging_) {
        return;
    }

    if (mode_ == ViewMode::Bar && !hoverExpanded_) {
        hoverExpanded_ = true;
        KillTimer(hwnd_, kTimerCollapse);
        ApplyModeLayout(true);
    }

    GetCursorPos(&dragCursorStart_);
    GetWindowRect(hwnd_, &dragWindowStart_);
    dragging_ = true;
    dragMoved_ = false;
    SetCapture(hwnd_);
}

void BallWindow::OnLButtonUp() {
    if (!dragging_) {
        return;
    }

    dragging_ = false;
    if (GetCapture() == hwnd_) {
        ReleaseCapture();
    }

    if (!dragMoved_) {
        return;
    }

    RECT rect{};
    GetWindowRect(hwnd_, &rect);
    ballPosition_.x = rect.left;
    ballPosition_.y = rect.top;

    TryDock(rect);
    if (dockEdge_ != 0) {
        mode_ = ViewMode::Bar;
        hoverExpanded_ = false;
    } else {
        mode_ = ViewMode::Ball;
        hoverExpanded_ = false;
    }

    ApplyModeLayout(true);
    SaveConfig();
}

void BallWindow::OnLButtonDblClk(POINT clientPoint) {
    if (mode_ == ViewMode::Bar || PtInRect(&memoryHitRect_, clientPoint)) {
        StartClean();
    }
}

void BallWindow::OnMouseLeave() {
    if (dragging_) {
        return;
    }
    if (mode_ == ViewMode::Bar && hoverExpanded_) {
        SetTimer(hwnd_, kTimerCollapse, 450, nullptr);
    }
}

void BallWindow::OnDpiChanged(UINT newDpi) {
    if (newDpi == 0) {
        return;
    }
    dpi_ = newDpi;
    ApplyModeLayout(true);
}

void BallWindow::OnCleanDone(unsigned long long freedMegabytes) {
    cleaning_ = false;
    UpdateMetrics();

    if (freedMegabytes == 0) {
        toast_ = L"清理完成";
    } else {
        toast_ = L"释放 " + FormatFreedCompact(freedMegabytes);
    }
    toastUntilMs_ = GetTickCount64() + 3200;
    Paint();
}

void BallWindow::ShowMenu(POINT screenPoint) {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }

    AppendMenuW(menu, MF_STRING, kCommandClean, L"立即清理内存");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCommandExit, L"退出");

    SetForegroundWindow(hwnd_);
    const int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                       screenPoint.x, screenPoint.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
    PostMessageW(hwnd_, WM_NULL, 0, 0);

    if (command == static_cast<int>(kCommandClean)) {
        StartClean();
    } else if (command == static_cast<int>(kCommandExit)) {
        DestroyWindow(hwnd_);
    }
}

void BallWindow::Paint() {
    if (hwnd_ == nullptr) {
        return;
    }

    RECT rect{};
    GetWindowRect(hwnd_, &rect);

    DrawParams params;
    params.width = rect.right - rect.left;
    params.height = rect.bottom - rect.top;
    params.barMode = (mode_ == ViewMode::Bar && !hoverExpanded_);
    params.verticalBar = params.barMode && (dockEdge_ == 1 || dockEdge_ == 2);
    params.dockEdge = dockEdge_;
    params.barThickness = Scale(kBarShortDip);
    params.mem = memory_;
    params.net = network_;
    params.cleaning = cleaning_;
    params.toast = toast_;
    params.dpi = dpi_;

    const POINT position{rect.left, rect.top};
    memoryHitRect_ = RenderWindow(hwnd_, params, position);
}

void BallWindow::UpdateMetrics() {
    memory_ = metrics_.SampleMemory();
    network_ = metrics_.SampleNetwork();

    if (toastUntilMs_ != 0 && GetTickCount64() >= toastUntilMs_) {
        toast_.clear();
        toastUntilMs_ = 0;
    }
}

void BallWindow::StartClean() {
    if (cleaning_) {
        return;
    }

    cleaning_ = true;
    toast_.clear();
    toastUntilMs_ = 0;
    Paint();

    HWND target = hwnd_;
    std::thread([target]() {
        const unsigned long long freed = CleanMemory();
        PostMessageW(target, kMessageCleanDone, static_cast<WPARAM>(freed), 0);
    }).detach();
}

void BallWindow::ApplyModeLayout(bool repaint) {
    const RECT work = WorkArea();
    const bool bar = (mode_ == ViewMode::Bar && !hoverExpanded_);
    const bool verticalBar = (dockEdge_ == 1 || dockEdge_ == 2);

    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;

    if (bar) {
        width = Scale(verticalBar ? kBarShortDip : kBarLongDip);
        height = Scale(verticalBar ? kBarLongDip : kBarShortDip);
        if (dockEdge_ == 1) {
            x = work.left;
            y = dockAlong_ - height / 2;
        } else if (dockEdge_ == 2) {
            x = work.right - width;
            y = dockAlong_ - height / 2;
        } else if (dockEdge_ == 3) {
            y = work.top;
            x = dockAlong_ - width / 2;
        } else if (dockEdge_ == 4) {
            y = work.bottom - height;
            x = dockAlong_ - width / 2;
        } else {
            x = ballPosition_.x;
            y = ballPosition_.y;
        }
    } else {
        width = Scale(kBallSizeDip);
        height = width;
        if (dockEdge_ != 0 && hoverExpanded_) {
            if (dockEdge_ == 1) {
                x = work.left;
                y = dockAlong_ - height / 2;
            } else if (dockEdge_ == 2) {
                x = work.right - width;
                y = dockAlong_ - height / 2;
            } else if (dockEdge_ == 3) {
                y = work.top;
                x = dockAlong_ - width / 2;
            } else if (dockEdge_ == 4) {
                y = work.bottom - height;
                x = dockAlong_ - width / 2;
            }
        } else {
            x = ballPosition_.x;
            y = ballPosition_.y;
        }
    }

    const int minX = work.left;
    const int minY = work.top;
    const int maxX = std::max(work.left, work.right - width);
    const int maxY = std::max(work.top, work.bottom - height);
    x = std::clamp(x, minX, maxX);
    y = std::clamp(y, minY, maxY);

    if (!bar) {
        ballPosition_.x = x;
        ballPosition_.y = y;
    }

    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    HRGN region = nullptr;
    if (bar) {
        const int radius = std::min(width, height);
        region = CreateRoundRectRgn(0, 0, width + 1, height + 1, radius, radius);
    } else {
        region = CreateEllipticRgn(0, 0, width + 1, height + 1);

        // 悬停展开时把不可见的贴边条区域并入命中区域，保证在条的两端点击/移动仍有效。
        if (region != nullptr && mode_ == ViewMode::Bar && hoverExpanded_ && dockEdge_ != 0) {
            const int shortSide = Scale(kBarShortDip);
            RECT barRect{};
            if (dockEdge_ == 1) {
                barRect = {0, 0, shortSide, height};
            } else if (dockEdge_ == 2) {
                barRect = {width - shortSide, 0, width, height};
            } else if (dockEdge_ == 3) {
                barRect = {0, 0, width, shortSide};
            } else if (dockEdge_ == 4) {
                barRect = {0, height - shortSide, width, height};
            }

            HRGN barRegion = CreateRoundRectRgn(barRect.left, barRect.top, barRect.right + 1,
                                                barRect.bottom + 1, shortSide, shortSide);
            if (barRegion != nullptr) {
                CombineRgn(region, region, barRegion, RGN_OR);
                DeleteObject(barRegion);
            }
        }
    }

    if (region != nullptr) {
        if (!SetWindowRgn(hwnd_, region, TRUE)) {
            DeleteObject(region);
        }
    }

    if (repaint) {
        Paint();
    }
}

void BallWindow::TryDock(const RECT& windowRect) {
    const RECT work = WorkArea();
    const int threshold = Scale(kDockThresholdDip);

    const int leftDistance =
        std::max(0, static_cast<int>(windowRect.left) - static_cast<int>(work.left));
    const int rightDistance =
        std::max(0, static_cast<int>(work.right) - static_cast<int>(windowRect.right));
    const int topDistance =
        std::max(0, static_cast<int>(windowRect.top) - static_cast<int>(work.top));
    const int bottomDistance =
        std::max(0, static_cast<int>(work.bottom) - static_cast<int>(windowRect.bottom));

    int bestDistance = threshold + 1;
    int bestEdge = 0;

    if (leftDistance < bestDistance) {
        bestDistance = leftDistance;
        bestEdge = 1;
    }
    if (rightDistance < bestDistance) {
        bestDistance = rightDistance;
        bestEdge = 2;
    }
    if (topDistance < bestDistance) {
        bestDistance = topDistance;
        bestEdge = 3;
    }
    if (bottomDistance < bestDistance) {
        bestDistance = bottomDistance;
        bestEdge = 4;
    }

    if (bestDistance > threshold) {
        dockEdge_ = 0;
        return;
    }

    dockEdge_ = bestEdge;
    if (bestEdge == 1 || bestEdge == 2) {
        dockAlong_ = windowRect.top + (windowRect.bottom - windowRect.top) / 2;
    } else {
        dockAlong_ = windowRect.left + (windowRect.right - windowRect.left) / 2;
    }
}

int BallWindow::Scale(int value) const {
    return MulDiv(value, static_cast<int>(dpi_), 96);
}

RECT BallWindow::WorkArea() const {
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    if (monitor != nullptr && GetMonitorInfoW(monitor, &info)) {
        return info.rcWork;
    }

    RECT fallback{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    return fallback;
}

std::wstring BallWindow::ConfigPath() const {
    wchar_t appData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);

    std::wstring directory;
    if (length > 0 && length < MAX_PATH) {
        directory = appData;
    } else {
        directory = L".";
    }

    directory += L"\\AccelBall";
    CreateDirectoryW(directory.c_str(), nullptr);
#ifdef ACCELBALL_DEV
    return directory + L"\\config-dev.ini";
#else
    return directory + L"\\config.ini";
#endif
}

void BallWindow::LoadConfig() {
    const RECT work = WorkArea();
    const int size = Scale(kBallSizeDip);

    ballPosition_.x = work.right - size - Scale(kDefaultMarginDip);
    ballPosition_.y = work.top + ((work.bottom - work.top) - size) / 2;
    mode_ = ViewMode::Ball;
    hoverExpanded_ = false;
    dockEdge_ = 0;
    dockAlong_ = 0;

    const std::wstring path = ConfigPath();
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        const int edge = ReadIniInt(path, L"window", L"edge", 0);
        const int along = ReadIniInt(path, L"window", L"along", 0);
        if (edge >= 1 && edge <= 4 && along != 0) {
            dockEdge_ = edge;
            dockAlong_ = along;
            mode_ = ViewMode::Bar;
        } else {
            ballPosition_.x = ReadIniInt(path, L"window", L"x", ballPosition_.x);
            ballPosition_.y = ReadIniInt(path, L"window", L"y", ballPosition_.y);
        }
    }

    if (mode_ == ViewMode::Ball) {
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        HMONITOR monitor = MonitorFromPoint(
            POINT{ballPosition_.x + size / 2, ballPosition_.y + size / 2},
            MONITOR_DEFAULTTONEAREST);
        if (monitor != nullptr && GetMonitorInfoW(monitor, &info)) {
            const LONG maxX = std::max<LONG>(info.rcWork.left, info.rcWork.right - size);
            const LONG maxY = std::max<LONG>(info.rcWork.top, info.rcWork.bottom - size);
            ballPosition_.x = std::clamp<LONG>(ballPosition_.x, info.rcWork.left, maxX);
            ballPosition_.y = std::clamp<LONG>(ballPosition_.y, info.rcWork.top, maxY);
        }
    }
}

void BallWindow::SaveConfig() {
    const std::wstring path = ConfigPath();
    wchar_t buffer[32];

    if (mode_ == ViewMode::Bar && dockEdge_ != 0) {
        swprintf_s(buffer, L"%d", dockEdge_);
        WritePrivateProfileStringW(L"window", L"edge", buffer, path.c_str());
        swprintf_s(buffer, L"%d", dockAlong_);
        WritePrivateProfileStringW(L"window", L"along", buffer, path.c_str());
        WritePrivateProfileStringW(L"window", L"mode", L"bar", path.c_str());
    } else {
        WritePrivateProfileStringW(L"window", L"edge", L"0", path.c_str());
        swprintf_s(buffer, L"%d", ballPosition_.x);
        WritePrivateProfileStringW(L"window", L"x", buffer, path.c_str());
        swprintf_s(buffer, L"%d", ballPosition_.y);
        WritePrivateProfileStringW(L"window", L"y", buffer, path.c_str());
        WritePrivateProfileStringW(L"window", L"mode", L"ball", path.c_str());
    }
}
