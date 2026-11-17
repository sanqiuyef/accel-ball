#include "render.h"

#include "util.h"

#include <gdiplus.h>

#include <algorithm>
#include <cstdio>
#include <string>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace {

Color UsageColor(unsigned int percent, BYTE alpha = 255) {
    if (percent < 65) {
        return Color(alpha, 34, 197, 94);
    }
    if (percent < 85) {
        return Color(alpha, 245, 158, 11);
    }
    return Color(alpha, 239, 68, 68);
}

const FontFamily* UiFamily() {
    // 故意不析构：静态对象会在 GdiplusShutdown 之后才析构，届时调用 GDI+ 会崩溃。
    static FontFamily* yahei = new FontFamily(L"Microsoft YaHei UI");
    if (yahei->IsAvailable()) {
        return yahei;
    }
    return FontFamily::GenericSansSerif();
}

void AddRoundRect(GraphicsPath& path, const RectF& rect, float radius) {
    const float diameter = radius * 2.0f;
    path.AddArc(rect.X, rect.Y, diameter, diameter, 180.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - diameter, rect.Y, diameter, diameter, 270.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - diameter, rect.Y + rect.Height - diameter, diameter, diameter,
                0.0f, 90.0f);
    path.AddArc(rect.X, rect.Y + rect.Height - diameter, diameter, diameter, 90.0f, 90.0f);
    path.CloseFigure();
}

void DrawBall(Graphics& graphics, const DrawParams& params, RECT* hitRect) {
    const float scale = static_cast<float>(params.dpi) / 96.0f;
    const float width = static_cast<float>(params.width);
    const float height = static_cast<float>(params.height);
    const float diameter = std::min(width, height);

    // 贴边悬停展开时，球的方形窗口会覆盖整条停靠条；条两端在圆形之外、alpha=0 时
    // 会被分层窗口当作穿透区域。这里先铺一层 alpha=1 的不可见底，恢复实时命中。
    if (params.dockEdge >= 1 && params.dockEdge <= 4 && params.barThickness > 0) {
        const float thickness = static_cast<float>(params.barThickness);
        RectF hitArea{};
        if (params.dockEdge == 1) {
            hitArea = RectF(0.0f, 0.0f, thickness, height);
        } else if (params.dockEdge == 2) {
            hitArea = RectF(width - thickness, 0.0f, thickness, height);
        } else if (params.dockEdge == 3) {
            hitArea = RectF(0.0f, 0.0f, width, thickness);
        } else {
            hitArea = RectF(0.0f, height - thickness, width, thickness);
        }

        GraphicsPath hitPath;
        AddRoundRect(hitPath, hitArea, thickness / 2.0f);
        SolidBrush hitFill(Color(1, 0, 0, 0));
        graphics.FillPath(&hitFill, &hitPath);
    }

    RectF circle(1.0f, 1.0f, diameter - 2.0f, diameter - 2.0f);
    SolidBrush background(Color(242, 26, 32, 44));
    graphics.FillEllipse(&background, circle);

    SolidBrush sheen(Color(16, 255, 255, 255));
    graphics.FillEllipse(&sheen, RectF(diameter * 0.20f, diameter * 0.06f, diameter * 0.60f,
                                       diameter * 0.22f));

    Pen border(Color(46, 255, 255, 255), 1.0f);
    graphics.DrawEllipse(&border, circle);

    const float inset = 3.5f * scale;
    const float ringWidth = 3.5f * scale;
    RectF ring(inset, inset, diameter - inset * 2.0f, diameter - inset * 2.0f);
    Pen track(Color(46, 255, 255, 255), ringWidth);
    graphics.DrawEllipse(&track, ring);

    if (params.mem.percent > 0) {
        Pen progress(UsageColor(params.mem.percent), ringWidth);
        progress.SetStartCap(LineCapRound);
        progress.SetEndCap(LineCapRound);
        const float sweep = std::min(359.9f, 360.0f * static_cast<float>(params.mem.percent) / 100.0f);
        graphics.DrawArc(&progress, ring, -90.0f, sweep);
    }

    const FontFamily* family = UiFamily();
    Font percentFont(family, 18.0f * scale, FontStyleBold, UnitPixel);
    Font smallFont(family, 9.0f * scale, FontStyleRegular, UnitPixel);
    Font toastFont(family, 9.0f * scale, FontStyleBold, UnitPixel);
    SolidBrush white(Color(255, 255, 255, 255));
    SolidBrush muted(Color(235, 210, 218, 228));
    SolidBrush accent(Color(255, 147, 197, 253));
    SolidBrush success(Color(255, 134, 239, 172));

    StringFormat centered;
    centered.SetAlignment(StringAlignmentCenter);
    centered.SetLineAlignment(StringAlignmentCenter);

    wchar_t percentText[16];
    swprintf_s(percentText, L"%u%%", params.mem.percent);
    graphics.DrawString(percentText, -1, &percentFont,
                        RectF(0.0f, height * 0.13f, width, height * 0.40f), &centered, &white);

    hitRect->left = static_cast<LONG>(width * 0.17f);
    hitRect->top = static_cast<LONG>(height * 0.11f);
    hitRect->right = static_cast<LONG>(width * 0.83f);
    hitRect->bottom = static_cast<LONG>(height * 0.55f);

    if (params.cleaning) {
        graphics.DrawString(L"清理中…", -1, &smallFont,
                            RectF(0.0f, height * 0.61f, width, height * 0.20f), &centered, &accent);
    } else if (!params.toast.empty()) {
        graphics.DrawString(params.toast.c_str(), -1, &toastFont,
                            RectF(0.0f, height * 0.61f, width, height * 0.20f), &centered, &success);
    } else {
        const std::wstring upText = L"↑ " + FormatSpeedCompact(params.net.upBps);
        const std::wstring downText = L"↓ " + FormatSpeedCompact(params.net.downBps);
        graphics.DrawString(upText.c_str(), -1, &smallFont,
                            RectF(0.0f, height * 0.57f, width, height * 0.13f), &centered, &muted);
        graphics.DrawString(downText.c_str(), -1, &smallFont,
                            RectF(0.0f, height * 0.70f, width, height * 0.13f), &centered, &muted);
    }
}

void DrawBar(Graphics& graphics, const DrawParams& params, RECT* hitRect) {
    const float scale = static_cast<float>(params.dpi) / 96.0f;
    const float width = static_cast<float>(params.width);
    const float height = static_cast<float>(params.height);

    const float radius = std::min(width, height) / 2.0f - 1.5f;
    RectF box(1.5f, 1.5f, width - 3.0f, height - 3.0f);
    GraphicsPath path;
    AddRoundRect(path, box, radius);

    SolidBrush background(Color(242, 26, 32, 44));
    graphics.FillPath(&background, &path);
    Pen border(Color(46, 255, 255, 255), 1.0f);
    graphics.DrawPath(&border, &path);

    const FontFamily* family = UiFamily();
    SolidBrush white(Color(255, 255, 255, 255));
    StringFormat centered;
    centered.SetAlignment(StringAlignmentCenter);
    centered.SetLineAlignment(StringAlignmentCenter);

    if (params.verticalBar) {
        const float dotSize = 7.0f * scale;
        const float dotX = (width - dotSize) / 2.0f;
        const float dotY = 5.0f * scale;
        SolidBrush dot(UsageColor(params.mem.percent));
        graphics.FillEllipse(&dot, dotX, dotY, dotSize, dotSize);

        Font font(family, 9.5f * scale, FontStyleRegular, UnitPixel);
        graphics.DrawString(L"内", -1, &font, RectF(0.0f, height * 0.20f, width, height * 0.16f),
                            &centered, &white);
        graphics.DrawString(L"存", -1, &font, RectF(0.0f, height * 0.37f, width, height * 0.16f),
                            &centered, &white);

        wchar_t text[16];
        swprintf_s(text, L"%u%%", params.mem.percent);
        graphics.DrawString(text, -1, &font, RectF(0.0f, height * 0.60f, width, height * 0.22f),
                            &centered, &white);
    } else {
        const float dotSize = 7.0f * scale;
        const float dotX = 6.0f * scale;
        const float dotY = (height - dotSize) / 2.0f;
        SolidBrush dot(UsageColor(params.mem.percent));
        graphics.FillEllipse(&dot, dotX, dotY, dotSize, dotSize);

        Font font(family, 9.0f * scale, FontStyleRegular, UnitPixel);
        StringFormat leftAligned;
        leftAligned.SetAlignment(StringAlignmentNear);
        leftAligned.SetLineAlignment(StringAlignmentCenter);

        wchar_t text[32];
        swprintf_s(text, L"内存 %u%%", params.mem.percent);
        const float textX = 15.5f * scale;
        graphics.DrawString(text, -1, &font,
                            RectF(textX, 0.0f, width - textX - 4.0f * scale, height), &leftAligned,
                            &white);
    }

    hitRect->left = 0;
    hitRect->top = 0;
    hitRect->right = static_cast<LONG>(width);
    hitRect->bottom = static_cast<LONG>(height);
}

}  // namespace

RECT RenderWindow(HWND hwnd, const DrawParams& params, POINT screenPosition) {
    RECT hitRect{};
    const int width = std::max(1, params.width);
    const int height = std::max(1, params.height);

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;  // 自上而下
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC screenDc = GetDC(nullptr);
    HDC memoryDc = CreateCompatibleDC(screenDc);
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(memoryDc, &bitmapInfo, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (dib == nullptr) {
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return hitRect;
    }

    HGDIOBJ previousBitmap = SelectObject(memoryDc, dib);
    {
        // 直接以 PARGB 格式绑定 DIB 内存，保证文本/图形写入预乘 alpha。
        Bitmap surface(width, height, width * 4, PixelFormat32bppPARGB,
                       static_cast<BYTE*>(bits));
        Graphics graphics(&surface);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
        graphics.SetCompositingMode(CompositingModeSourceOver);
        graphics.Clear(Color(0, 0, 0, 0));

        if (params.barMode) {
            DrawBar(graphics, params, &hitRect);
        } else {
            DrawBall(graphics, params, &hitRect);
        }
    }

    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    SIZE size{width, height};
    POINT source{0, 0};
    UpdateLayeredWindow(hwnd, screenDc, &screenPosition, &size, memoryDc, &source, 0, &blend,
                        ULW_ALPHA);

    SelectObject(memoryDc, previousBitmap);
    DeleteObject(dib);
    DeleteDC(memoryDc);
    ReleaseDC(nullptr, screenDc);
    return hitRect;
}
