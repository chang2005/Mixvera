# Mixvera 架构

`0.2.0` 采用「服务化内核 + Qt 前端 + 可替换媒体引擎」，但这一版把「能出画面」的前端补齐了：真实采集、场景合成、转场渲染与 ffmpeg 输出都在 UI 层完成，服务层仍保持无 Qt 依赖、可单测。

## 目录

```text
apps/mixvera          应用入口（--screenshot / --dump-ui / --project）
src/mixvera/core      领域模型、工程序列化、事件总线、服务容器、生命周期
src/mixvera/services  输入、切换、转场、音频、输出、回放、设备
src/mixvera/control   JSON-RPC 控制协议骨架（尚未绑定网络）
src/mixvera/engine    MediaEngine 抽象与 Stub 后端
src/mixvera/ui        Qt Widgets 界面
  ├─ media            采集提供者、合成器、ffmpeg 录制器、音频电平
  └─ platform         Win32 窗口枚举
tests                 core / project / ui / media
third_party           nlohmann/json
```

## 分层规则

1. `core` 不依赖 Qt：领域类型、工程 JSON、服务与事件总线都能在纯 C++ 测试里跑。
2. `ui` 不直接改 `SwitcherService` 以外的业务状态；切换、转场、音频都走服务层。
3. `ui/media` 是这一版的渲染核心：采集 → 缓存 → 合成 → 转场 → 显示/输出，一条时间线。
4. `MediaEngine` 仍是后端抽象点；当前 Stub，未来 `LibobsMediaEngine` 可替换 `ui/media` 的采集实现而不动界面。

## 一次显示 tick 的数据流

```text
MainWindow::tick()                      30 fps（可随工程 fps 变化）
  ├─ InputService::inputs()             取全部输入（含场景）
  ├─ Compositor::refresh(inputs)        每个 SourceProvider 取最新一帧，进帧缓存
  │    ├─ 屏幕/窗口：grabWindow
  │    ├─ 摄像头：QCamera + QMediaCaptureSession → QVideoSink
  │    ├─ 视频：QMediaPlayer → QVideoSink（循环）
  │    ├─ 图片：QImage
  │    └─ 标题/纯色/测试图案：QPainter 现画
  ├─ renderInput(program) / renderInput(preview)
  │    └─ 若为 Scene：按 items 顺序合成，几何用 0..1 归一化坐标
  ├─ 若转场中：mix(program, preview, kind, position)
  ├─ 若 FTB：fadeToBlack
  ├─ StageView::setFrame(PGM / PVW)
  └─ Recorder::submit(PGM)              → ffmpeg stdin（录制 / 推流）
```

缩略图每 6 个 tick 刷新一次（约 5 fps），只做缩放，不重新采集。

转场推进由同一个 tick 驱动：`AUTO` 用 `QElapsedTimer` 计算进度，`T-BAR` 由滑块值直接给进度；到达 100% 时调用 `SwitcherService::take()` 交换 PGM/PVW，并把进度归零。

## 领域模型

```c++
InputKind: Unknown | TestPattern | ScreenCapture | WindowCapture | Camera |
           Image | MediaFile | Title | Color | Scene | DeckLink | Ndi

InputDescriptor {
  id, name, kind, active
  params  // monitor / window / device / file / text / color / size / align
  items   // Scene only：{inputId, x, y, width, height, opacity, visible}
  hasAudio, volumeDb, muted
}

Project {
  name, canvas { width, height, fps }
  inputs[]
  programId, previewId
  transition { kind, durationMs }
  outputs { recordingDirectory, streamUrl, streamKey }
}
```

场景是 `kind == Scene` 的输入，因此「切换目标」统一是 input id：vMix 的输入切换与 OBS 的场景切换在同一套状态机里成立，`SwitcherService` 的既有测试不需要改写。

## 采集与合成

- `SourceProvider` 只有一个职责：返回最新一帧。设备类（摄像头、播放器）由回调更新缓存帧，调用方永远不阻塞。
- `Compositor` 持有 `id → provider` 与 `id → 最新帧` 两张表；`sync()` 只在输入种类或参数变化时重建 provider。
- 合成用 `drawCover`（等比铺满、居中裁切），图层透明度取自 `SceneItem::opacity`。
- 转场在帧层面实现：Fade 叠透明度，Wipe 裁剪矩形，Slide 平移两帧，Zoom 交叉缩放。

## 输出

`Recorder` 用 `QProcess` 启动 ffmpeg，输入是 `rawvideo/bgra` 原始帧，编码 `libx264`（不存在时回退 `mpeg4`）：

```text
ffmpeg -f rawvideo -pix_fmt bgra -s WxH -r FPS -i pipe:0 -an -c:v libx264 ...
```

录制与推流各一个进程，同一帧同时写入。停止时关闭 stdin 并等待 ffmpeg 收尾，保证 MP4 完整。

## 音频

`AudioMeter` 打开系统默认录音设备（`QAudioSource`，48 kHz / 单声道 / Int16），每个 tick 计算 RMS 转 dBFS，带快起慢落。混音器只展示主输出电平；每路推子与静音写入 `AudioService` 与工程，尚未参与输出混音。

## 工程持久化

`core/project.cpp` 负责 JSON 往返。`MainWindow` 在切换、增删输入、改设置后写回 `%APPDATA%/Mixvera/project.json`，退出时再写一次；启动读回，失败则回落到 `makeDefaultProject()`。

## 线程模型

全部在 UI 线程：屏幕抓取、合成、ffmpeg 写入都由定时器驱动。摄像头与视频帧通过 Qt 的信号进入缓存，不额外起线程。4K 屏幕采集在低端机器上可能掉帧，下一版会把采集与合成拆到工作线程。

## 与 libobs 的关系

`MIXVERA_ENABLE_LIBOBS=ON` 只影响 `mixvera_engine` 与链接。若接入 libobs，计划由 `LibobsMediaEngine` 提供 source/scene/transition 的真实实现，`ui/media` 的采集器退化为可选后端，界面与工程模型不变。分发需遵守 GPL-2.0-or-later。