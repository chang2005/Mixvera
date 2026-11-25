# Changelog

## [0.4.0] — 2026-11-25

主题、调音台与设置界面的完整设计；采集、合成、切换与输出沿用 0.3。

### 新增

- **三套主题**：`Charcoal`（vMix 炭灰 + 橙色强调，默认）、`Yami`（OBS `Yami.obt` 蓝灰 + `#284CB8`）、`Daylight`（浅色高对比）
- **token 化样式表**：47 个 `Palette` token 驱动一份 QSS 模板（`theme_stylesheet.hpp`）与全部自绘控件；不再有散落的颜色字面量
- **主题实时预览**：设置 → 常规的三张主题卡片带缩略图，选中即应用到主窗口，取消恢复原主题
- **主题选择入口**：`--theme`、`MIXVERA_THEME`、`QSettings ui/theme`；支持 `yami`/`obs`、`daylight`/`light` 别名
- **调音台重做**：OBS 式通道条（编号、名称、dB、竖向推子、`M`/`S`）、双电平平段主表、刻度与峰值保持、双击复位 0 dB、水平滚动
- **独奏**：`AudioChannelState::solo` 与 `audio/solo_changed`；非独奏通道条变暗，头部显示 `SOLO n`
- **设置界面重做**：左侧分类导航 + 六个分页（常规、视频、音频、输出、快捷键、关于）；主题、语言、恢复会话、画布、ffmpeg、RTMP 全部集中于此
- **跟随系统语言** 与 **启动时恢复上次会话** 成为真实设置
- **图标**：自绘 `chevron-down` / `chevron-up` / 浅色版 `check` SVG，用于下拉框、微调框与复选勾选
- **截图 CLI**：`--theme`、`--show-mixer`、`--no-restore`、`--settings-screenshot`、`--settings-page`
- 文档：新增 `docs/DESIGN.md`；`docs/UI-SPEC.md` 补充 0.4 主题/混音器/设置的证据映射

### 变更

- 默认工程中测试图案与纯色输入启用音频，首次启动调音台即有两条真实通道
- 翻译规模 213 → 305 条，新增主题名/描述、混音器与设置页文案，全部完成
- 版本号升至 0.4.0；窗口标题更新
- 关于对话框补充 Qt、编译器、平台、许可与设计参考

### 修复

- QSS 模板中的占位符残留与强调色上的深色文字对比度
- `QDialogButtonBox` 先 polish 后命名导致「确定」按钮未被主按钮样式覆盖（改为命名后 unpolish/polish）
- 设置窗口截图后进程不退出（截图后显式 `close()` 再 `quit()`）
- 推子槽在 Daylight 主题下对比度不足（改用 `borderStrong` 与内描边）
- 电平表顶端刻度被裁切、`M`/`S` 文字过暗

### 测试

- UI smoke 新增主题用例：三套样式表非空、无残留 `{{token}}`、彼此不同、键别名正确、`daylight` 应用到主窗口
- 四个测试目标全部通过（core / project / ui smoke / media）

## [0.3.0] — 2026-11-25

简体中文界面 + vMix 式工作区重排。采集、合成、切换与输出能力沿用 0.2。

### 新增

- **简体中文**：213 条界面文案全部翻译；`QTranslator` 从内嵌资源 `:/i18n/mixvera_zh_CN.qm` 加载
- **语言选择**：`设置 → 界面语言`、`--lang zh_CN`、`MIXVERA_LANG`、`QSettings ui/language`、系统语言五级回退
- **vMix 式布局**：预览左 / 节目右双监视器，中央竖向转场列，底部横向输入条与功能栏
- **输入条**：190×164 缩略图卡片、PGM/PVW 标题色、PVW / PGM / 加场景 / 属性 / 移除快捷按钮，分类筛选与名称搜索
- **可收起面板**：场景与来源、调音台均可从顶栏或「视图」菜单显示/隐藏
- **vMix 配色**：`#1E2328` 背景、`#404B56` 转场面板、`#FF8C00` 预览、`#006400` 节目、`#293038` 页脚
- **翻译资源**：`translations/mixvera_zh_CN.ts`、`mixvera_zh_CN.qm`、`translations.qrc`
- `--lang` 命令行参数与 `--dump-ui` 中文控件树输出

### 变更

- 主窗口由「左侧场景/源 + 大 PGM/小 PVW + 右侧输入列表」改为「可收起场景栏 + PVW/转场/PGM 三联 + 横向输入条 + 页脚」
- 转场按钮改为 `硬切` / `自动` 上下排列，Fade / Wipe / Slide / Zoom 2×2 网格，T-BAR 改为竖向推子
- 监视器标题条改为 vMix 式纯色条：预览橙、节目绿，不再覆盖画面
- 调音台从底部横排改为右侧可收起竖排，通道条更紧凑
- 输入列表由 `QListWidget` 改为 `QScrollArea` + 自绘 `InputTile`
- 默认工程名称（Welcome / Camera 1 / Scene 1 等）随界面语言本地化
- 测试改为通过 `InputPanel::count()` 校验输入数量

### 修复

- `lupdate` 生成的 context 缺少 `mixvera::ui::` 命名空间，导致 `.qm` 能加载但界面不翻译；现已在 `.ts` 中补齐
- 中文界面下 QSS 复选框缺少对勾图标，新增 `resources/check.svg`
- 输入条在窗口缩小时横向滚动而不是压缩缩略图

### 测试

- 四个测试目标全部通过（core / project / ui smoke / media）
- UI smoke 覆盖新输入条结构、CUT/AUTO/T-BAR、监视器出帧、场景选择与界面录制

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