# 加速球（AccelBall）

Windows 10 桌面悬浮球小工具，借鉴杀毒/电脑管家的「加速球」交互形态，只做四件事：

1. 常驻圆形悬浮球，显示当前内存占用百分比。
2. 双击球上的内存百分比区域，执行一次内存清理并刷新数值。
3. 实时显示当前主机的网络上传 / 下载速率。
4. 拖到屏幕边缘松手自动收起为一条只显示内存占用的横条；鼠标移入展开为圆球，移出后重新收起。

不包含病毒查杀、进程管理、开机自启、皮肤、托盘菜单等功能。

## 运行环境

- Windows 10 x64（19045 实测）。
- 程序以管理员权限运行（manifest `requireAdministrator`），用于回收全系统进程工作集并清理 standby list。

## 使用

- 拖动圆球：按住左键拖动。
- 清理内存：双击球中央的百分比区域；横条形态下双击横条任意位置。
- 贴边收起：把球拖到任意屏幕边缘松手，自动吸附并变为横条。
- 恢复圆球：鼠标移入横条。
- 退出 / 立即清理：右键球或横条。

## 构建

需要 Visual Studio 2022 Build Tools（MSVC x64）与 Windows SDK。构建脚本会调用：

```
D:\VS2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat
```

```bash
# 开发版（asInvoker，便于无需 UAC 自测）
build\build.bat dev

# 发布版（requireAdministrator，交付用）
build\build.bat release
```

产物输出到 `dist/`：`AccelBall-dev.exe` 或 `AccelBall.exe`。

## 安装包

见 `installer/accel-ball.iss`，用 Inno Setup 6 编译。安装包输出到 `dist/`。

## 内存清理机制

参照 Mem Reduct / WinMemoryCleaner / 360 加速球的常见做法，自研实现（未复制 GPL 代码）：

1. 启用 `SeDebugPrivilege`、`SeProfileSingleProcessPrivilege`、`SeIncreaseQuotaPrivilege`。
2. 遍历系统进程，对可打开的进程调用 `EmptyWorkingSet`，把工作集页移出。
3. 通过 `NtSetSystemInformation(SystemMemoryListInformation, MemoryPurgeStandbyList)` 清理 standby list。
4. 前后对比 `GlobalMemoryStatusEx` 的可用物理内存，显示释放量。

注意：内存清理会把页移到 standby / 换出，并不保证程序变快；这是此类工具的普遍行为。清理在后台线程执行，不阻塞界面。

## 网速口径

使用 `GetIfTable2` 读取 64 位累计字节数，按 1 秒采样差值计算。只统计处于 Up 状态的物理接口（`HardwareInterface`），跳过 loopback、隧道和虚拟网卡，避免 VPN / Hyper-V 场景重复计数。单位自动切换 B/s、KB/s、MB/s。

## 目录

- `src/`：C++ 源码与资源（manifest、图标）。
- `build/`：构建脚本。
- `installer/`：Inno Setup 脚本。
- `tools/`：图标生成脚本。
- `docs/`：自测记录与截图。

## 现状

开发中，详见 `docs/自测记录.md`。
