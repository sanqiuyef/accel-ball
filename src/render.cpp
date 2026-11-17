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

void DrawBall(Graphics& graphics, const DrawParams& params, RECT* hitRect) {
    const float scale = static_cast<float>(params.dpi) / 96.0f;
    const float width = static_cast<float>(params.width);
    const float height = static_cast<float>(params.height);
    const float diameter = std::min(width, height);

    RectF circle(1.5f, 1.5f, diameter - 3.0f, diameter - 3.0f);
    SolidBrush background(Color(242, 26, 32, 44));
    graphics.FillEllipse(&background, circle);

    SolidBrush sheen(Color(16, 255, 255, 255));
    graphics.FillEllipse(&sheen, RectF(diameter * 0.17f, diameter * 0.055f,
                                       diameter * 0.66f, diameter * 0.30f));

    Pen border(Color(46, 255, 255, 255), 1.0f);
    graphics.DrawEllipse(&border, circle);

    const float inset = 6.0f * scale;
    RectF ring(inset, inset, diameter - inset * 2.0f, diameter - inset * 2.0f);
    Pen track(Color(46, 255, 255, 255), 5.0f * scale);
    graphics.DrawEllipse(&track, ring);

    if (params.mem.percent > 0) {
        Pen progress(UsageColor(params.mem.percent), 5.0f * scale);
        progress.SetStartCap(LineCapRound);
        progress.SetEndCap(LineCapRound);
        const float sweep = std::min(359.9f, 360.0f * static_cast<float>(params.mem.percent) / 100.0f);
        graphics.DrawArc(&progress, ring, -90.0f, sweep);
    }

    const FontFamily* family = UiFamily();
    Font percentFont(family, 31.0f * scale, FontStyleBold, UnitPixel);
    Font smallFont(family, 11.5f * scale, FontStyleRegular, UnitPixel);
    SolidBrush white(Color(255, 255, 255, 255));
    SolidBrush muted(Color(235, 210, 218, 228));
    SolidBrush accent(Color(255, 147, 197, 253));
    SolidBrush success(Color(255, 134, 239, 172));

    StringFormat centered;
    centered.SetAlignment(StringAlignmentCenter);
    centered.SetLineAlignment(StringAlignmentCenter);

    wchar_t percentText[16];
    swprintf_s(percentText, L"%u%%", params.mem.percent);
    RectF percentRect(0.0f, height * 0.20f, width, height * 0.40f);
    graphics.DrawString(percentText, -1, &percentFont, percentRect, &centered, &white);

    hitRect->left = static_cast<LONG>(width * 0.14f);
    hitRect->top = static_cast<LONG>(height * 0.16f);
    hitRect->right = static_cast<LONG>(width * 0.86f);
    hitRect->bottom = static_cast<LONG>(height * 0.62f);

    if (params.cleaning) {
        RectF messageRect(0.0f, height * 0.66f, width, height * 0.16f);
        graphics.DrawString(L"正在清理…", -1, &smallFont, messageRect, &centered, &accent);
    } else if (!params.toast.empty()) {
        Font toastFont(family, 11.0f * scale, FontStyleBold, UnitPixel);
        RectF messageRect(0.0f, height * 0.66f, width, height * 0.16f);
        graphics.DrawString(params.toast.c_str(), -1, &toastFont, messageRect, &centered, &success);
    } else {
        const std::wstring upText = L"↑ " + FormatSpeed(params.net.upBps);
        const std::wstring downText = L"↓ " + FormatSpeed(params.net.downBps);
        graphics.DrawString(upText.c_str(), -1, &smallFont,
                            RectF(0.0f, height * 0.625f, width, height * 0.115f), &centered, &muted);
        graphics.DrawString(downText.c_str(), -1, &smallFont,
                            RectF(0.0f, height * 0.748f, width, height * 0.115f), &centered, &muted);
    }
}

void DrawBar(Graphics& graphics, const DrawParams& params, RECT* hitRect) {
    const float scale = static_cast<float>(params.dpi) / 96.0f;
    const float width = static_cast<float>(params.width);
    const float height = static_cast<float>(params.height);

    const float corner = (height - 3.0f) / 2.0f;
    RectF box(1.5f, 1.5f, width - 3.0f, height - 3.0f);

    GraphicsPath path;
    path.AddArc(box.X, box.Y, corner * 2.0f, corner * 2.0f, 180.0f, 90.0f);
    path.AddArc(box.X + box.Width - corner * 2.0f, box.Y, corner * 2.0f, corner * 2.0f, 270.0f, 90.0f);
    path.AddArc(box.X + box.Width - corner * 2.0f, box.Y + box.Height - corner * 2.0f,
                corner * 2.0f, corner * 2.0f, 0.0f, 90.0f);
    path.AddArc(box.X, box.Y + box.Height - corner * 2.0f, corner * 2.0f, corner * 2.0f, 90.0f, 90.0f);
    path.CloseFigure();

    SolidBrush background(Color(242, 26, 32, 44));
    graphics.FillPath(&background, &path);
    Pen border(Color(46, 255, 255, 255), 1.0f);
    graphics.DrawPath(&border, &path);

    const float dotSize = 9.0f * scale;
    const float dotX = 12.0f * scale;
    const float dotY = (height - dotSize) / 2.0f;
    SolidBrush dot(UsageColor(params.mem.percent));
    graphics.FillEllipse(&dot, dotX, dotY, dotSize, dotSize);

    const FontFamily* family = UiFamily();
    Font font(family, 12.0f * scale, FontStyleRegular, UnitPixel);
    SolidBrush white(Color(255, 255, 255, 255));
    StringFormat leftAligned;
    leftAligned.SetAlignment(StringAlignmentNear);
    leftAligned.SetLineAlignment(StringAlignmentCenter);

    wchar_t text[32];
    swprintf_s(text, L"内存 %u%%", params.mem.percent);
    const float textX = dotX + dotSize + 7.0f * scale;
    RectF textRect(textX, 0.0f, width - textX - 8.0f * scale, height);
    graphics.DrawString(text, -1, &font, textRect, &leftAligned, &white);

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
