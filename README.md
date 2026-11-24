# Mixvera

Mixvera 是一个面向专业导播场景的开源切换台软件基础框架。当前仓库包含 `0.1.0` 基础框架，媒体引擎默认使用 Stub 后端，可在尚未配置 libobs 的环境直接构建与运行。

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

在 “x64 Native Tools Command Prompt for VS 2022” 中执行：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.12.1/msvc2022_64" -DMIXVERA_ENABLE_LIBOBS=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

启动：

```powershell
.\build\Mixvera.exe
```

## libobs 模式

准备好 libobs 的 CMake package 后：

```powershell
cmake -S . -B build-libobs -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.12.1/msvc2022_64;C:/path/to/obs/build" -DMIXVERA_ENABLE_LIBOBS=ON
cmake --build build-libobs
```

注意：当前仅建立构建开关和引擎抽象，尚未实现 `LibobsMediaEngine` 具体适配层。
