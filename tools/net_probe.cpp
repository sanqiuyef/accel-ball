// 网络计数诊断工具：打印每块接口的过滤条件与累计字节，按秒输出过滤后的速率。
// 用法：net_probe.exe [秒数]
#include "../src/common.h"

#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <ipifcons.h>

#include <cstdio>
#include <cstdlib>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

int wmain(int argc, wchar_t** argv) {
    const int seconds = argc > 1 ? static_cast<int>(wcstol(argv[1], nullptr, 10)) : 10;
    unsigned long long previousIn = 0;
    unsigned long long previousOut = 0;
    unsigned long long previousTick = 0;

    for (int tick = 0; tick < seconds; ++tick) {
        PMIB_IF_TABLE2 table = nullptr;
        unsigned long long sumIn = 0;
        unsigned long long sumOut = 0;

        if (GetIfTable2(&table) == NO_ERROR && table != nullptr) {
            for (ULONG i = 0; i < table->NumEntries; ++i) {
                const MIB_IF_ROW2& row = table->Table[i];
                const bool hardware = row.InterfaceAndOperStatusFlags.HardwareInterface != 0;
                const bool up = row.OperStatus == IfOperStatusUp;
                const bool included = hardware && up && row.Type != IF_TYPE_SOFTWARE_LOOPBACK &&
                                      row.Type != IF_TYPE_TUNNEL;

                if (tick == 0) {
                    wprintf(L"if[%lu] alias=%s desc=%s type=%lu up=%d hw=%d in=%llu out=%llu include=%d\n",
                            i, row.Alias, row.Description, static_cast<unsigned long>(row.Type),
                            up ? 1 : 0, hardware ? 1 : 0, row.InOctets, row.OutOctets,
                            included ? 1 : 0);
                }

                if (included) {
                    sumIn += row.InOctets;
                    sumOut += row.OutOctets;
                }
            }
            FreeMibTable(table);
        }

        const unsigned long long now = GetTickCount64();
        if (tick > 0 && now > previousTick) {
            const double secondsElapsed = static_cast<double>(now - previousTick) / 1000.0;
            wprintf(L"t=%d down=%.0f up=%.0f B/s sum_in=%llu sum_out=%llu\n", tick,
                    static_cast<double>(sumIn - previousIn) / secondsElapsed,
                    static_cast<double>(sumOut - previousOut) / secondsElapsed, sumIn, sumOut);
            fflush(stdout);
        }

        previousIn = sumIn;
        previousOut = sumOut;
        previousTick = now;
        Sleep(1000);
    }
    return 0;
}
