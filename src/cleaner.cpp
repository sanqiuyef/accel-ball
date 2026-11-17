#include "cleaner.h"

#include <psapi.h>

#include <chrono>
#include <thread>
#include <vector>

#pragma comment(lib, "psapi.lib")

namespace {

constexpr ULONG kSystemMemoryListInformation = 80;
constexpr ULONG kMemoryPurgeStandbyList = 4;

using NtSetSystemInformationFn = LONG(NTAPI*)(ULONG, PVOID, ULONG);

void EnablePrivilege(const wchar_t* name) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        return;
    }

    LUID luid{};
    if (LookupPrivilegeValueW(nullptr, name, &luid)) {
        TOKEN_PRIVILEGES privileges{};
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Luid = luid;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr);
    }

    CloseHandle(token);
}

void TrimProcessWorkingSets() {
    std::vector<DWORD> pids(1024);
    DWORD count = 0;

    for (;;) {
        DWORD needed = 0;
        if (!EnumProcesses(pids.data(), static_cast<DWORD>(pids.size() * sizeof(DWORD)), &needed)) {
            return;
        }
        if (needed < pids.size() * sizeof(DWORD)) {
            count = needed / sizeof(DWORD);
            break;
        }
        if (pids.size() >= 65536) {
            return;
        }
        pids.resize(pids.size() * 2);
    }

    const DWORD self = GetCurrentProcessId();
    for (DWORD i = 0; i < count; ++i) {
        const DWORD pid = pids[i];
        if (pid == 0 || pid == 4 || pid == self) {
            continue;
        }

        HANDLE process = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (process == nullptr) {
            process = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, pid);
        }
        if (process != nullptr) {
            EmptyWorkingSet(process);
            CloseHandle(process);
        }
    }
}

void PurgeStandbyList() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        return;
    }

    auto ntSetSystemInformation =
        reinterpret_cast<NtSetSystemInformationFn>(GetProcAddress(ntdll, "NtSetSystemInformation"));
    if (ntSetSystemInformation == nullptr) {
        return;
    }

    ULONG command = kMemoryPurgeStandbyList;
    ntSetSystemInformation(kSystemMemoryListInformation, &command, sizeof(command));
}

MemSnapshot CurrentMemory() {
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

}  // namespace

unsigned long long CleanMemory() {
    EnablePrivilege(L"SeDebugPrivilege");
    EnablePrivilege(L"SeProfileSingleProcessPrivilege");
    EnablePrivilege(L"SeIncreaseQuotaPrivilege");

    const MemSnapshot before = CurrentMemory();

    TrimProcessWorkingSets();
    PurgeStandbyList();

    // 给内存子系统一点时间刷新可用内存计数。
    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    const MemSnapshot after = CurrentMemory();
    if (after.availPhys <= before.availPhys) {
        return 0;
    }
    return (after.availPhys - before.availPhys) / (1024ull * 1024ull);
}
