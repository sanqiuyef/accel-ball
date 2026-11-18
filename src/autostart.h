#pragma once

#include "common.h"

#include <string>

// 查询计划任务是否已注册（存在即视为已开启自启）。
bool AutostartTaskExists(const std::wstring& taskName);

// 开启时创建“登录时、最高权限、交互式”的计划任务；关闭时删除。
bool SetAutostartTask(const std::wstring& taskName, const std::wstring& exePath, bool enabled);
