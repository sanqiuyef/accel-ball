#pragma once

// 注意：不能定义 WIN32_LEAN_AND_MEAN，否则 GDI+ 依赖的 OLE 类型（IStream/PROPID）会缺失。
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// winsock2.h 必须先于 windows.h，否则 iphlpapi.h 里的 MIB_IF_ROW2/GetIfTable2 不可用。
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>

#include <cstdint>
#include <string>

struct MemSnapshot {
    unsigned long long totalPhys = 0;
    unsigned long long availPhys = 0;
    unsigned int percent = 0;
};

struct NetSpeed {
    double upBps = 0.0;
    double downBps = 0.0;
};
