# AccelBall 文件夹结构

```text
accel-ball/
├─ src/                  C++ 源码与资源
│  ├─ main.cpp           程序入口、GDI+ 初始化、单实例
│  ├─ ball_window.*      悬浮窗窗口过程、交互、配置
│  ├─ metrics.*          内存与网速采样
│  ├─ cleaner.*          内存清理（工作集 + standby list）
│  ├─ render.*           74px 圆球 / 30×74 竖条 / 74×30 横条绘制
│  ├─ util.*             速率/释放量格式化（含紧凑单位）
│  ├─ app.manifest       发布版 manifest（requireAdministrator）
│  ├─ app-dev.manifest   开发版 manifest（asInvoker）
│  ├─ app.rc             发布版资源
│  ├─ app-dev.rc         开发版资源
│  ├─ resource.h         资源 ID
│  └─ app.ico            程序图标（由 tools/make_icon.py 生成）
├─ build/
│  ├─ build.bat          vcvars64 + rc + cl 构建 dev/release
│  └─ build_probe.bat    构建网络计数诊断探针
├─ installer/
│  └─ accel-ball.iss     Inno Setup 6 安装脚本
├─ tools/
│  ├─ make_icon.py       生成 app.ico
│  └─ net_probe.cpp      打印每块接口过滤条件与速率合计
├─ docs/
│  ├─ 自测记录.md        构建与功能自测记录
│  └─ screenshots/       界面截图（本地留档，git 忽略）
├─ dist/                 构建产物（git 忽略）
│  ├─ AccelBall.exe      发布版（管理员权限）
│  ├─ AccelBall-dev.exe  开发版（无需 UAC，独立 config-dev.ini）
│  ├─ net_probe.exe      网络计数诊断工具
│  ├─ AccelBall-Setup-1.0.0.exe  安装包
│  └─ obj/               中间产物
├─ README.md             项目说明
└─ FOLDER_STRUCTURE.md   本文件
```
