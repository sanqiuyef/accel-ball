# AccelBall 文件夹结构

```text
accel-ball/
├─ src/                  C++ 源码与资源
│  ├─ main.cpp           程序入口、GDI+ 初始化、单实例
│  ├─ ball_window.*      悬浮窗窗口过程、交互、配置
│  ├─ metrics.*          内存与网速采样
│  ├─ cleaner.*          内存清理（工作集 + standby list）
│  ├─ render.*           圆球/横条绘制与分层窗口提交
│  ├─ util.*             速率/释放量格式化
│  ├─ app.manifest       发布版 manifest（requireAdministrator）
│  ├─ app-dev.manifest   开发版 manifest（asInvoker）
│  ├─ app.rc             发布版资源
│  ├─ app-dev.rc         开发版资源
│  ├─ resource.h         资源 ID
│  └─ app.ico            程序图标（由 tools/make_icon.py 生成）
├─ build/
│  └─ build.bat          调用 vcvars64 + rc + cl 构建
├─ installer/
│  └─ accel-ball.iss     Inno Setup 6 安装脚本
├─ tools/
│  └─ make_icon.py       生成 app.ico
├─ docs/
│  ├─ 自测记录.md        构建与功能自测记录
│  └─ screenshots/       界面截图
├─ dist/                 构建产物（git 忽略）
├─ README.md             项目说明
└─ FOLDER_STRUCTURE.md   本文件
```
