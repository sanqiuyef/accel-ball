#pragma once

#include "common.h"

struct DrawParams {
    int width = 0;
    int height = 0;
    bool barMode = false;
    bool verticalBar = false;
    int dockEdge = 0;
    int barThickness = 0;
    MemSnapshot mem;
    NetSpeed net;
    bool cleaning = false;
    std::wstring toast;
    UINT dpi = 96;
};

// 绘制并以分层窗口方式提交，返回内存百分比区域的命中矩形（客户区坐标）。
RECT RenderWindow(HWND hwnd, const DrawParams& params, POINT screenPosition);
