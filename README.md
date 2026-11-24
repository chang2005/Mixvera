# Mixvera

Mixvera 是一个面向专业导播场景的开源切换台软件。当前仓库为 `0.1.0` 基础框架与第一版可运行 Demo，媒体引擎默认使用 Stub 后端，可在尚未配置 libobs 的环境直接构建与运行。

![Mixvera demo](docs/demo/mixvera-demo.png)

## 第一版 Demo

已实现 PGM/PVW 双监视器、MultiView 点击预览、CUT/AUTO/T-Bar 转场、音频推子与 6 路总线路由、三路输出启停、Replay 控制和快捷键操作。详细说明与操作方式见 [docs/DEMO.md](docs/DEMO.md)。

当前画面为程序生成的模拟信号，真实 SDI/NDI 输入与 RTMP/NDI/录制输出将在 libobs 适配层接入。

## 技术路线

- 最小 Fork libobs，保持可重放的上游补丁
- C++17 服务化内核
- Qt 6 Widgets 自研前端
- PGM/PVW、MultiView、T-Bar、混音、输出、回放、设备的服务层抽象
- JSON-RPC 远程控制协议骨架
- 媒体引擎接口支持 Stub 与 libobs 后端

## 目录结构

详见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)。

## Windows 构建

已验证环境：Qt 6.8.3 MinGW、MinGW 13.1、CMake 4.x、Ninja。

```powershell
cmake --preset windows-mingw
cmake --build --preset windows-mingw
ctest --test-dir build/mingw --output-on-failure
```

启动：

```powershell
.\build\mingw\Mixvera.exe
```

`windows-mingw` 预设中的 Qt、MinGW 与 Ninja 路径位于 `CMakePresets.json`，可按本机安装位置调整。

## libobs 模式

准备好 libobs 的 CMake package 后：

```powershell
cmake -S . -B build-libobs -G Ninja -DCMAKE_PREFIX_PATH="D:/Qt/6.8.3/mingw_64;C:/path/to/obs/build" -DMIXVERA_ENABLE_LIBOBS=ON
cmake --build build-libobs
```

注意：当前仅建立构建开关和引擎抽象，尚未实现 `LibobsMediaEngine` 具体适配层。
