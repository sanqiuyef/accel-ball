#pragma once

#include "common.h"

// 将字节/秒格式化为 B/s、KB/s、MB/s。
std::wstring FormatSpeed(double bytesPerSecond);

// 将 MB 数格式化为 MB 或 GB。
std::wstring FormatFreed(unsigned long long megabytes);
