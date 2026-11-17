#pragma once

#include "common.h"

// 执行一次内存清理：回收可访问进程的工作集，并清理 standby list。
// 返回清理后可用物理内存的增量（MB）。需要在管理员权限下运行才能获得完整效果。
unsigned long long CleanMemory();
