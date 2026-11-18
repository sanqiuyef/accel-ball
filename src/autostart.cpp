#include "autostart.h"

#include <vector>

namespace {

std::wstring Quote(const std::wstring& value) {
    return L"\"" + value + L"\"";
}

std::wstring SystemCommandPath() {
    wchar_t directory[MAX_PATH]{};
    const UINT length = GetSystemDirectoryW(directory, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::wstring(directory, length) + L"\\schtasks.exe";
    }
    return L"C:\\Windows\\System32\\schtasks.exe";
}

std::wstring CurrentUser() {
    wchar_t domain[256]{};
    wchar_t user[256]{};
    const DWORD domainLength = GetEnvironmentVariableW(L"USERDOMAIN", domain, 256);
    const DWORD userLength = GetEnvironmentVariableW(L"USERNAME", user, 256);
    if (userLength == 0 || userLength >= 256) {
        return L"";
    }

    std::wstring result;
    if (domainLength > 0 && domainLength < 256) {
        result = std::wstring(domain, domainLength) + L"\\";
    }
    result += std::wstring(user, userLength);
    return result;
}

bool RunHidden(const std::wstring& commandLine, DWORD timeoutMs) {
    std::vector<wchar_t> buffer(commandLine.begin(), commandLine.end());
    buffer.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, buffer.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, nullptr, &startup, &process)) {
        return false;
    }

    const DWORD wait = WaitForSingleObject(process.hProcess, timeoutMs);
    bool success = false;
    if (wait == WAIT_OBJECT_0) {
        DWORD exitCode = 1;
        if (GetExitCodeProcess(process.hProcess, &exitCode)) {
            success = exitCode == 0;
        }
    } else if (wait == WAIT_TIMEOUT) {
        TerminateProcess(process.hProcess, 1);
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return success;
}

}  // namespace

bool AutostartTaskExists(const std::wstring& taskName) {
    const std::wstring command =
        Quote(SystemCommandPath()) + L" /Query /TN " + Quote(taskName);
    return RunHidden(command, 5000);
}

bool SetAutostartTask(const std::wstring& taskName, const std::wstring& exePath, bool enabled) {
    if (!enabled) {
        const std::wstring command =
            Quote(SystemCommandPath()) + L" /Delete /TN " + Quote(taskName) + L" /F";
        return RunHidden(command, 10000);
    }

    if (exePath.empty()) {
        return false;
    }

    const std::wstring user = CurrentUser();
    if (user.empty()) {
        return false;
    }

    // schtasks 的 /TR 参数需要把带空格的 exe 路径再包一层引号：
    // /TR "\"C:\Program Files\AccelBall\AccelBall.exe\""
    const std::wstring taskRun = L"\\\"" + exePath + L"\\\"";
    const std::wstring command = Quote(SystemCommandPath()) + L" /Create /TN " + Quote(taskName) +
                                 L" /TR " + Quote(taskRun) + L" /SC ONLOGON /RL HIGHEST /RU " +
                                 Quote(user) + L" /IT /F";
    return RunHidden(command, 15000);
}
