# Changelog

## [0.2.0] — 2026-11-24

第一个可用版本：真实采集、场景合成、转场与录制全部打通，界面按 OBS Studio 与 vMix 的可观察产物重做。

### 新增

- **真实输入**：屏幕采集、窗口采集、摄像头、图片、视频文件、标题、纯色、测试图案
- **场景与图层**：场景增删改复制；源列表显隐、上移/下移、移出场景；归一化图层几何
- **合成与转场**：`Compositor` 统一合成 PGM/PVW；Fade / Wipe / Slide / Zoom；AUTO 定时推进；T-BAR 手动推子；FTB
- **vMix 式输入列表**：实时缩略图、编号、类型、PGM/PVW/静音标记、右键菜单
- **输出**：ffmpeg 原始帧管道录制 MP4、RTMP 推流；REC/STREAM 计时胶囊；ON AIR 状态
- **音频**：真实输入设备电平表（RMS → dBFS），通道推子与静音
- **工程**：JSON 序列化、自动保存/恢复、`--project` 载入
- **界面**：OBS Yami 配色重制主题；深色对话框与滚动区；`--dump-ui` 控件树导出

### 变更

- 默认工程改为两个场景（测试图案 + 标题、纯色 + 半透明标题），PGM/PVW 指向 SCENE 1 / SCENE 2
- 移除 0.1.0 的模拟信号监视器与独立 MultiView 面板，改由真实采集与合成管线驱动
- `InputDescriptor` 增加 `params`、`items`、`hasAudio`、`volumeDb`、`muted`
- `TransitionKind` 增加 Slide / Zoom，移除 Stinger（无对应实现）

### 修复

- 场景列表程序化选中会再次触发 `currentItemChanged` 造成重建递归
- 场景属性对话框会按输入类型误改场景类型
- 推流 URL 与串流密钥可能重复拼接
- 混音器滚动区继承 Qt 默认浅色背景导致大面积发白

### 测试

- 新增 `mixvera_project_tests`（工程 JSON 往返、默认工程、图层几何）
- 新增 `mixvera_media_tests`（屏幕采集出帧、窗口枚举、合成非黑、ffmpeg 录制）
- 扩充 `mixvera_ui_smoke_tests`（默认工程、CUT/AUTO/T-BAR、监视器出帧、场景选择、界面录制落盘）

## [0.1.0] — 2026-11-24

- 服务化内核、Stub 媒体引擎、Qt 界面骨架
- PGM/PVW 模拟监视器、CUT/AUTO/T-Bar、音频推子、输出启停、Replay 骨架