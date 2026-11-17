#pragma once

#include "common.h"

// 内存与网络速率采样器。SampleNetwork 依赖两次采样的时间差，建议每秒调用一次。
class Metrics {
public:
    MemSnapshot SampleMemory() const;
    NetSpeed SampleNetwork();

private:
    bool hasPrevious_ = false;
    unsigned long long previousInBytes_ = 0;
    unsigned long long previousOutBytes_ = 0;
    unsigned long long previousTickMs_ = 0;
};
