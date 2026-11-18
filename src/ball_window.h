#pragma once

#include "common.h"
#include "metrics.h"
#include "render.h"

#include <string>

class BallWindow {
public:
    bool Create(HINSTANCE instance);
    void Run();

private:
    enum class ViewMode {
        Ball,
        Bar,
    };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void OnCreate();
    void OnDestroy();
    void OnTimer(UINT_PTR timerId);
    void OnMouseMove();
    void OnLButtonDown();
    void OnLButtonUp();
    void OnLButtonDblClk(POINT clientPoint);
    void OnMouseLeave();
    void OnDpiChanged(UINT newDpi);
    void OnCleanDone(unsigned long long freedMegabytes);
    void OnAutostartDone(bool enabled, bool success);
    void StartAutostartToggle(bool enabled);
    std::wstring AutostartTaskName() const;

    void ShowMenu(POINT screenPoint);
    void Paint();
    void UpdateMetrics();
    void StartClean();
    void ApplyModeLayout(bool repaint);
    void TryDock(const RECT& windowRect);
    int Scale(int value) const;
    RECT WorkArea() const;
    void LoadConfig();
    void SaveConfig();
    std::wstring ConfigPath() const;

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    UINT dpi_ = 96;

    Metrics metrics_;
    MemSnapshot memory_;
    NetSpeed network_;

    ViewMode mode_ = ViewMode::Ball;
    bool hoverExpanded_ = false;
    int dockEdge_ = 0;   // 0=未贴边, 1=左, 2=右, 3=上, 4=下
    int dockAlong_ = 0;  // 贴边后沿边的中心坐标（屏幕像素）
    POINT ballPosition_{0, 0};

    RECT memoryHitRect_{};
    bool dragging_ = false;
    bool dragMoved_ = false;
    POINT dragCursorStart_{};
    RECT dragWindowStart_{};

    bool cleaning_ = false;
    std::wstring toast_;
    unsigned long long toastUntilMs_ = 0;
    bool autostartEnabled_ = false;
    bool autostartBusy_ = false;
};
