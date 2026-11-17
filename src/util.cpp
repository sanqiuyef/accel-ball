#include "util.h"

#include <cstdio>

std::wstring FormatSpeed(double bytesPerSecond) {
    wchar_t buffer[64];
    if (bytesPerSecond >= 1024.0 * 1024.0) {
        swprintf_s(buffer, L"%.1f MB/s", bytesPerSecond / (1024.0 * 1024.0));
    } else if (bytesPerSecond >= 1024.0) {
        swprintf_s(buffer, L"%.0f KB/s", bytesPerSecond / 1024.0);
    } else if (bytesPerSecond >= 1.0) {
        swprintf_s(buffer, L"%.0f B/s", bytesPerSecond);
    } else {
        swprintf_s(buffer, L"0 KB/s");
    }
    return buffer;
}

std::wstring FormatFreed(unsigned long long megabytes) {
    wchar_t buffer[64];
    if (megabytes >= 1024) {
        swprintf_s(buffer, L"%.1f GB", static_cast<double>(megabytes) / 1024.0);
    } else {
        swprintf_s(buffer, L"%llu MB", megabytes);
    }
    return buffer;
}

std::wstring FormatSpeedCompact(double bytesPerSecond) {
    wchar_t buffer[32];
    if (bytesPerSecond >= 1024.0 * 1024.0) {
        swprintf_s(buffer, L"%.1fM/s", bytesPerSecond / (1024.0 * 1024.0));
    } else if (bytesPerSecond >= 1024.0) {
        swprintf_s(buffer, L"%.0fK/s", bytesPerSecond / 1024.0);
    } else if (bytesPerSecond >= 1.0) {
        swprintf_s(buffer, L"%.0fB/s", bytesPerSecond);
    } else {
        swprintf_s(buffer, L"0K/s");
    }
    return buffer;
}

std::wstring FormatFreedCompact(unsigned long long megabytes) {
    wchar_t buffer[32];
    if (megabytes >= 1024) {
        swprintf_s(buffer, L"%.1fG", static_cast<double>(megabytes) / 1024.0);
    } else {
        swprintf_s(buffer, L"%lluM", megabytes);
    }
    return buffer;
}
