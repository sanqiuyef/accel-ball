// 自启逻辑诊断工具：在管理员终端中测试计划任务的查询/创建/删除。
// 用法：autostart_probe.exe <query|on|off> <taskName> [exePath]
#include "../src/autostart.h"

#include <cstdio>
#include <string>

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) {
        wprintf(L"usage: autostart_probe.exe <query|on|off> <taskName> [exePath]\n");
        return 2;
    }

    const std::wstring mode = argv[1];
    const std::wstring taskName = argv[2];
    const std::wstring exePath = argc > 3 ? argv[3] : L"";

    if (mode == L"query") {
        wprintf(L"exists=%d\n", AutostartTaskExists(taskName) ? 1 : 0);
    } else if (mode == L"on") {
        wprintf(L"set=%d\n", SetAutostartTask(taskName, exePath, true) ? 1 : 0);
    } else if (mode == L"off") {
        wprintf(L"set=%d\n", SetAutostartTask(taskName, exePath, false) ? 1 : 0);
    } else {
        wprintf(L"unknown mode\n");
        return 2;
    }
    return 0;
}
