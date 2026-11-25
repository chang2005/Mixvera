# Mixvera 架构

`0.6.0` 沿用「服务化内核 + Qt 前端 + 可替换媒体引擎」：真实采集、色度键控、场景合成、转场渲染、GT 标题包装与 ffmpeg 输出都在 UI 层完成，服务层保持无 Qt 依赖、可单测。中文界面通过 Qt `QTranslator` 加载内嵌 `.qm`；0.5 的叠加层与标题、0.6 的竖屏画布、抠像、标题包装与命名预置都不改变服务层接口。

## 目录

```text
apps/mixvera          应用入口（--screenshot / --dump-ui / --project）
src/mixvera/core      领域模型、工程序列化、事件总线、服务容器、生命周期
src/mixvera/services  输入、切换、转场、音频、输出、回放、设备
src/mixvera/control   JSON-RPC 控制协议骨架（尚未绑定网络）
src/mixvera/engine    MediaEngine 抽象与 Stub 后端
src/mixvera/ui        Qt Widgets 界面
  ├─ media            采集提供者、色度键控、GT 标题包装 / 二维码、合成器、ffmpeg 录制器、音频电平
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
  ├─ Compositor::sync(inputs)           输入变化时重建 provider，并下发画布尺寸
  ├─ Compositor::refresh(inputs)        每个 SourceProvider 取最新一帧，进帧缓存
  │    ├─ 屏幕/窗口：grabWindow
  │    ├─ 摄像头：QCamera + QMediaCaptureSession → QVideoSink
  │    ├─ 视频：QMediaPlayer → QVideoSink（循环）
  │    ├─ 图片：QImage
  │    ├─ 标题：内置样式 QPainter 现画；包装样式由 TitlePackage::render 栅格化
  │    ├─ 纯色/测试图案：QPainter 现画
  │    └─ 若输入开启抠像：applyChromaKey(帧) → 预乘 ARGB
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
  params  // monitor / window / device / file / text / color / size / align /
          // captions / tickerSpeed / chroma*（键控参数）/
          // package / titleText:* / titleColor:* / titleImage:* /
          // titlePreset:* / titlePresetActive / titlePresetChanged
  items   // Scene only：{inputId, x, y, width, height, opacity, visible}
  hasAudio, volumeDb, muted
}

Project {
  name, canvas { width, height, fps }   // 竖屏 / 横屏 / 方形由宽高决定
  inputs[]
  programId, previewId
  transition { kind, durationMs }
  overlays[4] { layer, enabled, input }
  outputs { recordingDirectory, streamUrl, streamKey }
}
```

场景是 `kind == Scene` 的输入，因此「切换目标」统一是 input id：vMix 的输入切换与 OBS 的场景切换在同一套状态机里成立，`SwitcherService` 的既有测试不需要改写。

## 采集与合成

- `SourceProvider` 只有一个职责：返回最新一帧。设备类（摄像头、播放器）由回调更新缓存帧，调用方永远不阻塞。
- `Compositor` 持有 `id → provider` 与 `id → 最新帧` 两张表；`sync()` 只在输入种类或参数变化时重建 provider。
- 合成用 `drawCover`（等比铺满、居中裁切），图层透明度取自 `SceneItem::opacity`。
- 转场在帧层面实现：Fade 叠透明度，Wipe 裁剪矩形，Slide 平移两帧，Zoom 交叉缩放。

## 画布、抠像与字幕

- **画布**：`Project::canvas` 是唯一尺寸来源。`MainWindow::setCanvasSize()` 钳制宽高、更新工程与「视图 → 竖屏画布」勾选状态、调用 `Compositor::setCanvasSize()`、刷新监视器并保存；设置对话框与 `--canvas` 只是入口。录制、推流与快照都消费合成器输出，因此输出尺寸自动跟随画布。
- **画布感知的 provider**：`SourceProvider::setCanvasSize(QSize)` 默认空实现；`TitleProvider` 用它计算 `unit = 画布宽 / 1280`，把字体、边距、卡片与滚动速度按比例缩放，竖屏下字幕仍落在画面内。
- **抠像**：`ui/media/chroma_key.{hpp,cpp}` 定义 `ChromaKeySettings`、`inputSupportsChromaKey()` 与 `applyChromaKey()`。`Compositor::refresh()` 对屏幕 / 窗口 / 摄像头 / 图片 / 视频帧应用键控，再写入帧缓存；输出为 `Format_ARGB32_Premultiplied`，后续 `QPainter` 直接合成。算法比较归一化 RGB 色度（不是原始 Cb/Cr），相似度决定阈值、平滑度决定羽化、溢色抑制压掉键色通道。
- **参数稳定性**：`Compositor::providerParams()` 会剔除所有 `chroma*` 键，因此调抠像参数不会触发摄像头或采集 provider 重建，画面不闪断。
- **字幕**：标题样式 `caption` / `ticker` 由 `TitleProvider` 绘制；`captions` 为多行预置文案，`captionIndex` 指向当前行，输入条右键逐句切换时同时写回 `captionIndex` 与 `text`，随工程持久化。
- **代价**：抠像在 UI 线程逐帧处理，1080p 下每帧约 200 万像素；与采集、合成共用一条时间线，低端机器上 4K 输入会掉帧，后续把采集、键控与合成移到工作线程。

## 标题包装与预置

- **包模型**：`ui/media/title_package.{hpp,cpp}` 的 `TitlePackage` 是只读文档模型，`load()` 支持 `.gtzip`（内置只读 ZIP 读取器读 `document.xml` / `resources.xml`，stored / deflated，拒绝加密与 Zip64）、未压缩目录与散装 `.xaml` / `.sinsamtitle`；`render(canvas, textOverrides, colourOverrides, imageOverrides, animationSeconds, tickerSeconds, countdowns, dataChangeSeconds)` 返回透明背景的 `QImage`。
- **解析**：`nodeFrom()` 递归文档树，把 `Type` / `Dimensions` / `Location` / `Transform` / `Brush` / `Storyboard` 变成 `Node` / `BrushData` / `AnimationData`；`storyboardFrom()` 只保留入口、`TransitionOut`、`DataChangeIn`、`DataChangeOut` 四类，`Continuous` / `Page1` 不参与。`Transform Rotate` 取 `x,y,z` 的 z 分量，绝对值 >6.5 视为角度，否则按弧度。
- **绘制**：`drawNode()` 对容器与叶子统一应用动画状态（透明度、位移、缩放、旋转、Reveal 裁切）；`drawTextNode()` 处理字体 / 对齐 / 换行，`drawImageNode()` 处理铺满 / 等比，`drawTickerNode()` 递归取模板里最宽的 TextBlock 并水平或竖向滚动，`drawQrNode()` 调用 `qr_encoder`。
- **编码**：`qr_encoder.{hpp,cpp}` 实现 ISO/IEC 18004 字节模式（ECC M、版本 1–10、RS GF(256)、交错、对齐图形、格式 / 版本 BCH、8 种掩码惩罚选择），返回模块矩阵；超过 213 字节返回空。
- **时钟**：`titlePackageClockText()` 用 `ClockParts` 解析 .NET 自定义格式，`{0:...}` 支持大小写 `H/h`、`m`、`s`、`f/F`、`t/tt`、`d/M/y`、引号 / 转义 / `%`，倒计时使用 `|` 后段。
- **在 provider 中**：`TitleProvider::applyParams()` 收集 `titleText:` / `titleColor:` / `titleImage:` 与 `titlePresetChanged`，按需 `TitlePackage::load()`；`packageFrame()` 用 `packageClock_` / `tickerClock_` 驱动动画，预置切换时用 `presetChangedMs_` 选择 `DataChangeIn`。`update()` 在样式与包装路径不变时原地生效，`Compositor::sync()` 借它保留动画时钟。
- **预置**：`MainWindow::onTitleAction()` 的 presetSave / presetNext / presetPrev / presetClear 把全部元素覆盖快照到 `titlePreset:<n>:...`；应用前擦除当前覆盖，写入预置后再写 `titlePresetActive` 与 `titlePresetChanged`。预置随工程 JSON 持久化。

## 输出

`Recorder` 用 `QProcess` 启动 ffmpeg，输入是 `rawvideo/bgra` 原始帧，编码 `libx264`（不存在时回退 `mpeg4`）；`WxH` 取自工程画布，竖屏时就是 `1080x1920`：

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