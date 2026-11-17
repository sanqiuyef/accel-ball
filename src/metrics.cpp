#include "metrics.h"

#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <ipifcons.h>

#pragma comment(lib, "iphlpapi.lib")

MemSnapshot Metrics::SampleMemory() const {
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);

    MemSnapshot snapshot;
    if (GlobalMemoryStatusEx(&status)) {
        snapshot.totalPhys = status.ullTotalPhys;
        snapshot.availPhys = status.ullAvailPhys;
        snapshot.percent = status.dwMemoryLoad;
    }
    return snapshot;
}

NetSpeed Metrics::SampleNetwork() {
    NetSpeed speed;

    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR || table == nullptr) {
        return speed;
    }

    unsigned long long inBytes = 0;
    unsigned long long outBytes = 0;
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_IF_ROW2& row = table->Table[i];

        // 只统计物理网卡，跳过 loopback、隧道、VPN/Hyper-V 等虚拟接口，避免重复计数。
        if (!row.InterfaceAndOperStatusFlags.HardwareInterface) {
            continue;
        }
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK || row.Type == IF_TYPE_TUNNEL) {
            continue;
        }
        if (row.OperStatus != IfOperStatusUp) {
            continue;
        }

        inBytes += row.InOctets;
        outBytes += row.OutOctets;
    }
    FreeMibTable(table);

    const unsigned long long nowMs = GetTickCount64();
    if (hasPrevious_ && nowMs > previousTickMs_ &&
        inBytes >= previousInBytes_ && outBytes >= previousOutBytes_) {
        const double seconds = static_cast<double>(nowMs - previousTickMs_) / 1000.0;
        if (seconds > 0.05) {
            speed.downBps = static_cast<double>(inBytes - previousInBytes_) / seconds;
            speed.upBps = static_cast<double>(outBytes - previousOutBytes_) / seconds;
        }
    }

    previousInBytes_ = inBytes;
    previousOutBytes_ = outBytes;
    previousTickMs_ = nowMs;
    hasPrevious_ = true;
    return speed;
}
