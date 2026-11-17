#pragma once

#include "common.h"

// 将字节/秒格式化为 B/s、KB/s、MB/s。
std::wstring FormatSpeed(double bytesPerSecond);

// 小尺寸悬浮球使用的紧凑速率，例如 9K/s、1.2M/s。
std::wstring FormatSpeedCompact(double bytesPerSecond);

// 将 MB 数格式化为 MB 或 GB。
std::wstring FormatFreed(unsigned long long megabytes);

// 小尺寸提示使用的紧凑释放量，例如 512M、2.7G。
std::wstring FormatFreedCompact(unsigned long long megabytes);
