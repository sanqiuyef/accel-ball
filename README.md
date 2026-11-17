# 加速球（AccelBall）

Windows 10 桌面悬浮球小工具，借鉴杀毒/电脑管家的「加速球」交互形态，只做四件事：

1. 常驻圆形悬浮球，显示当前内存占用百分比。
2. 双击球上的内存百分比区域，执行一次内存清理并刷新数值。
3. 实时显示当前主机的网络上传 / 下载速率。
4. 拖到屏幕边缘松手自动收起为一条只显示内存占用的横条；鼠标移入展开为圆球，移出后重新收起。

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

- 拖动圆球：按住左键拖动。
- 清理内存：双击球中央的百分比区域；横条形态下双击横条任意位置。
- 贴边收起：把球拖到任意屏幕边缘松手，自动吸附并变为横条。
- 恢复圆球：鼠标移入横条。
- 退出 / 立即清理：右键球或横条。
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

产物输出到 `dist/`：`AccelBall-dev.exe`（约 226 KB）或 `AccelBall.exe`（约 226 KB）。

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

使用 `GetIfTable2` 读取 64 位累计字节数，按 1 秒采样差值计算。只统计处于 Up 状态的物理接口（`HardwareInterface`），跳过 loopback、隧道和虚拟网卡，避免 VPN / Hyper-V 场景重复计数。单位自动切换 B/s、KB/s、MB/s。

## 已知限制

- 每次从资源管理器启动都会弹 UAC；这是管理员权限要求的直接结果。
- 多显示器与 125% / 150% DPI 已按 PerMonitorV2 实现，但本机 100% 单屏只做了基础验证。
- 安装包未做静默安装/覆盖升级的专项测试。
- 未做开机自启、托盘图标、皮肤、多语言等扩展。

## 目录

- `src/`：C++ 源码与资源（manifest、图标）。
- `build/`：构建脚本。
- `installer/`：Inno Setup 脚本。
- `tools/`：图标生成脚本。
- `docs/`：自测记录。
- `dist/`：构建产物（git 忽略）。

自测详情见 [docs/自测记录.md](docs/自测记录.md)。
