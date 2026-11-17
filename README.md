# 加速球（AccelBall）

Windows 10 桌面悬浮球小工具，借鉴杀毒/电脑管家的「加速球」交互形态，只做四件事：

1. 常驻圆形悬浮球，显示当前内存占用百分比。
2. 双击球上的内存百分比区域，执行一次内存清理并刷新数值。
3. 实时显示当前主机的网络上传 / 下载速率。
4. 拖到屏幕边缘松手自动收起为一条只显示内存占用的贴边条（左右边缘竖向、上下边缘横向）；鼠标移入展开为圆球，移出后重新收起。

不包含病毒查杀、进程管理、开机自启、皮肤、托盘菜单等功能。

## 运行要求

- Windows 10 x64（19045 实测）。
- 程序以管理员权限运行（manifest `requireAdministrator`），用于回收全系统进程工作集并清理 standby list。从资源管理器双击会弹 UAC；从安装包安装后勾选「运行」不会再弹第二次。

## 安装 / 使用

安装包：`dist/AccelBall-Setup-1.0.0.exe`

- 安装到 `C:\Program Files\AccelBall`，创建开始菜单快捷方式（桌面快捷方式为可选项）。
- 绿色版：`dist/AccelBall.exe`，可直接运行。
- 卸载：设置 → 应用 → 加速球，或开始菜单里的卸载项。

操作：

- 球径 74px（原 124px 设计的 60%），圆球内显示百分比、上行、下行；网速用 `K/s`、`M/s` 紧凑单位。
- 拖动圆球：按住左键拖动。
- 清理内存：双击球中央的百分比区域；贴边条形态下双击贴边条任意位置。
- 贴边收起：把球拖到任意屏幕边缘松手，自动吸附。左右边缘变为 30×74 竖向条，上下边缘变为 74×30 横向条。
- 恢复圆球：鼠标移入贴边条；移出后约 0.45 秒重新收起。
- 退出 / 立即清理：右键球或贴边条。
- 窗口位置和贴边状态保存在 `%APPDATA%\AccelBall\config.ini`。

## 构建

需要 Visual Studio 2022 Build Tools（MSVC x64）与 Windows SDK。构建脚本固定调用：

```
D:\VS2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat
```

```bash
# 开发版（asInvoker，便于无需 UAC 自测）
build\build.bat dev

# 发布版（requireAdministrator，交付用）
build\build.bat release
```

产物输出到 `dist/`：`AccelBall-dev.exe`（约 227 KB）或 `AccelBall.exe`（约 227 KB）。

开发版使用独立的单实例名和 `%APPDATA%\AccelBall\config-dev.ini`，可与发布版并行运行。开发版还支持用环境变量强制缩放做布局自测（不写入发布版）：

```bash
set ACCELBALL_DPI=144   # 150% 缩放；发布版忽略此变量
dist\AccelBall-dev.exe
```

## 网络计数诊断

`tools/net_probe.cpp`（由 `build\build_probe.bat` 构建）会打印每块接口的 `HardwareInterface`、状态、累计字节与过滤结果，并按秒输出过滤后的合计速率，用于核对悬浮球显示：

```bash
build\build_probe.bat
dist\net_probe.exe 10
```

## 安装包

`installer/accel-ball.iss`，Inno Setup 6 编译（本机 6.7.3）：

```bash
"%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" installer\accel-ball.iss
```

输出 `dist/AccelBall-Setup-1.0.0.exe`。安装向导为英文（Inno Setup 未内置简体中文语言文件），应用名与快捷方式为中文。

## 内存清理机制

参照 Mem Reduct / WinMemoryCleaner / 360 加速球的常见做法，自研实现（未复制 GPL 代码）：

1. 启用 `SeDebugPrivilege`、`SeProfileSingleProcessPrivilege`、`SeIncreaseQuotaPrivilege`。
2. 遍历系统进程，对可打开的进程调用 `EmptyWorkingSet`，把工作集页移出。
3. 通过 `NtSetSystemInformation(SystemMemoryListInformation, MemoryPurgeStandbyList)` 清理 standby list。
4. 前后对比 `GlobalMemoryStatusEx` 的可用物理内存，在球上显示释放量（约 3 秒）。

清理在后台线程执行，不阻塞界面。注意：这类清理会把页移到 standby 或换出，并不保证程序变快，效果因系统当前状态而异；这是「加速球」类工具的普遍行为。

## 网速口径

使用 `GetIfTable2` 读取 64 位累计字节数，按 1 秒采样差值计算。只统计处于 Up 状态的物理接口（`HardwareInterface`），跳过 loopback、隧道和虚拟网卡；本机实测只有 WLAN（Intel AX200）进入求和，FlClash/Meta Tunnel 与 WFP/QoS 过滤接口均被排除，没有重复计数。

显示的是**全主机物理网卡合计**，不是某个进程的流量。限速下载对照时，curl 自身被限制在约 2 MB/s，但同一秒 WLAN 计数曾跳到约 23.7 MB/s；探针确认这是其他进程的真实流量，界面显示 22.3 M/s 与系统计数一致。单位自动切换 B/s、K/s、M/s。

## 已知限制

- 每次从资源管理器启动都会弹 UAC；这是管理员权限要求的直接结果。
- DPI 布局已用开发版的 `ACCELBALL_DPI=96/120/144/192` 强制缩放验证（球径 74/93/111/148，内存约 11.6 MB），但本机只有 100% 单屏，真实「跨显示器 DPI 切换」未实测；`WM_DPICHANGED`、`WM_DISPLAYCHANGE`、任务栏工作区变化均已接到重新布局。
- 多显示器硬件场景未实测；代码按最近显示器工作区定位并夹取，显示器移除/分辨率变化会重新布局。
- 未做清理前后的性能 A/B 测试；内存清理不保证提速。
- 未做开机自启、托盘图标、皮肤、多语言等扩展。

## 目录

- `src/`：C++ 源码与资源（manifest、图标）。
- `build/`：构建脚本。
- `installer/`：Inno Setup 脚本。
- `tools/`：图标生成脚本、网络计数诊断探针。
- `docs/`：自测记录。
- `dist/`：构建产物（git 忽略）。

自测详情见 [docs/自测记录.md](docs/自测记录.md)。
