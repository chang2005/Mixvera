# Mixvera 设计说明（0.6）

这份文档说明 0.6 的界面、主题、调音台与设置界面的设计依据和实现约定，以及 0.5–0.6 新增的叠加层、转场、标题、竖屏画布、抠像、字幕、GT 标题包装与显示模式。配色与控件语义的取证过程见 [UI-SPEC.md](UI-SPEC.md)，三份原程序（OBS、vMix、SinSam）与 vMix GT 模板库的观察记录见 [RE-PROJECTS.md](RE-PROJECTS.md) 与 [RE-SINSAM.md](RE-SINSAM.md)。

原则只有一句：**借鉴公开可观察的组织方式与颜色值，不复制任何专有代码、图标、字体或资源。**

## 设计目标

1. **现场可用**：高频操作（硬切、自动、推子、静音）都在第一屏、单手可及的位置；状态颜色（预览橙、节目绿、录制红）在任何主题下都不变。
2. **专业密度，而不是装饰密度**：参照 vMix「主窗体只有少量高频按钮，设置窗体承载全部配置」的分层，主窗口只放工作区与常驻状态，低频配置收进设置。
3. **一切主题化**：界面里不出现散落的颜色字面量；所有绘制与 QSS 都读取同一套 token，三套主题共享同一份样式表模板。
4. **诚实**：界面只显示真实存在的数据。电平表跟随真实输入设备；音频尚未混入录制时，调音台底部直接写明。

## 主题系统

### 结构

```
theme::Palette（47 个 token + 3 个图标 URL）
        │
        ├─ theme::styleSheet(id)   ← theme_stylesheet.hpp 的 {{token}} 模板
        └─ 自绘控件（stage_view / mixer LevelMeter / ThemePreview）
```

- `ThemeId` 三个取值：`Charcoal`（默认，vMix 式炭灰 + 橙色控制强调）、`Yami`（OBS 式蓝灰 + 蓝色强调）、`Daylight`（浅色高对比，供明亮导播间）。
- `theme_stylesheet.hpp` 里是一份完整 QSS，颜色、图标全部写成 `{{window}}`、`{{accent}}`、`{{chevronDown}}` 这类占位符；`theme::styleSheet(id)` 一次性替换成对应主题的值。
- 自绘控件不使用字面量：`stage_view.cpp` 的监视器底色、预览/节目标题条，`mixer_panel.cpp` 的电平表渐变，`settings_dialog.cpp` 的主题缩略图，都调用 `theme::palette()`。

### 关键 token

| Token | Charcoal | Yami | Daylight | 用途 |
| --- | --- | --- | --- | --- |
| `window` | `#1E2328` | `#13141A` | `#EEF1F5` | 主窗口底色 |
| `surface` | `#262C33` | `#1D1F26` | `#F7F9FB` | 面板、卡片 |
| `raised` | `#333B44` | `#272A33` | `#DCE3EB` | 按钮、输入条底 |
| `field` | `#1A1F24` | `#101116` | `#FFFFFF` | 输入框、下拉框 |
| `monitor` | `#0B0D12` | `#08090C` | `#0B0D12` | 监视器画面底（三主题都保持近黑） |
| `border` / `borderStrong` | `#3E4853` / `#4A5561` | `#323540` / `#3C404D` | `#C7D0DB` / `#AEB9C6` | 描边；`borderStrong` 用于推子槽等需要清晰边界的部件 |
| `text` / `textMuted` | `#E7EAED` / `#8E98A3` | `#E1E1E1` / `#969696` | `#1B2430` / `#5F6C7B` | 正文 / 次要说明 |
| `accent` / `accentHover` | `#FF8C00` / `#FFB45C` | `#284CB8` / `#476BD7` | `#C2410C` / `#EA580C` | 主按钮、选中、焦点 |
| `preview` | `#FF8C00` | `#F59E0B` | `#C2410C` | 预览标题条与 PVW 状态 |
| `program` | `#006400` | `#15803D` | `#15803D` | 节目标题条与 PGM 状态 |
| `danger` | `#C01C37` | `#C01C37` | `#B91C1C` | 录制、ON AIR、危险操作 |
| `meterLow/Mid/High/Peak` | `#25A231` / `#37D247` / `#E5AF24` / `#C01C37` | 同左 | `#16A34A` / `#22C55E` / `#D97706` / `#DC2626` | 电平表分段 |
| `onAccent` / `onPreview` / `onProgram` | `#14181C` / `#14181C` / `#FFFFFF` | `#FFFFFF` / `#14181C` / `#FFFFFF` | `#FFFFFF` ×3 | 强调色上的文字；橙色底在深色主题上用深色字，浅色主题用白字 |

Yami 的 `#13141A`、`#1D1F26`、`#272A33`、`#284CB8`、`#C01C37`、`#37D247`、`#E5AF24` 直接来自 OBS `Yami.obt` 的 `--grey8/7/6`、`--blue3`、`--red3`、`--green2`、`--yellow2`。Charcoal 的窗口、转场面板与信号色来自 vMix 帮助截图的像素采样（见 UI-SPEC 的「vMix 主窗口布局与配色」）。

### 图标

勾选框与下拉/微调箭头各有一份深色版（`check.svg`、`chevron-down.svg`、`chevron-up.svg`，用于 Charcoal）与浅色版（`*-light.svg`，用于 Yami/Daylight），由 `Palette::checkIcon` / `chevronDown` / `chevronUp` 指向 `:/icons/…`。图标是自绘 SVG，没有取用任何第三方资源。

### 运行顺序

1. `main.cpp` 读 `--theme`，其次环境变量 `MIXVERA_THEME`，写入 `qApp` 属性 `mixvera/theme`（可为空）。
2. `MainWindow` 构造时先取该属性，为空再读 `QSettings` 的 `ui/theme`，仍为空则 Charcoal。
3. `applyTheme(id)` 调用 `theme::setTheme` 与 `setStyleSheet(theme::styleSheet(id))`；打开设置对话框时主题可实时预览，取消则恢复进入前的主题。
4. 接受设置后写回 `ui/theme`。生效时立即对主窗口与输出画布重设样式表。

键别名：`yami` 与 `obs` 等价，`daylight` 与 `light` 等价，其余一律回退 Charcoal。

## 主窗口布局

默认 1600×960，最小 1280×760。

| 区域 | 尺寸 | 内容 |
| --- | --- | --- |
| 顶栏 | 高 52 | 品牌、工程名、面板开关（场景/调音台）、设置、全屏、REC / STREAM 胶囊 |
| 场景 / 来源栏 | 宽 220–320，可收起 | OBS 式场景列表 + 图层列表（显隐、上移/下移、移除） |
| 预览监视器 | 弹性 | 橙色标题条 `PREVIEW` |
| 转场列 | 固定 156 | 硬切 / 自动 / 六种转场 / FTB / T-BAR / 时长 |
| 节目监视器 | 弹性 | 绿色标题条 `PROGRAM` |
| 调音台 | 宽 280–520，可收起 | 见下一节 |
| 输入条 | 固定高 212 | 横向滚动缩略图、编号、名称、PGM/PVW、分类筛选与搜索 |
| 功能栏 | 固定高 54 | 添加输入、录制、推流、ON AIR / STANDBY |

监视器之间的间距 6 px，同一行内左右对称；预览/节目标题条用信号色，正文用 `onPreview` / `onProgram` 保证对比度。

## 调音台

参照 OBS 的通道条模型（名称 → 推子 → 电平/数值 → 静音/监听），按 vMix 的竖向条形态排布。面板头部是 `AUDIO MIXER` 标题、当前音频设备胶囊（过长时省略到 176 px）和 SOLO 计数胶囊。

每条通道（宽 74 px）：

1. 两位编号（`01`、`02`…，按有音频的输入顺序）
2. 省略到 64 px 的输入名（完整名在 tooltip）
3. 当前增益 dB 文本（`-60` … `+12`，0 显示 `0.0`）
4. 竖向推子 `QSlider#channelFader`，范围 -60…+12 dB，双击复位到 0 dB
5. `M` / `S` 两个 28×20 切换按钮（静音 / 独奏）

主通道（固定宽 92 px）：`MASTER` 标题、带刻度的电平表（0 / -6 / -12 / -24 / -48）、峰值保持线、dB 读数、`PGM` 标签。表体渐变按 OBS 电平分段（绿 → 黄 → 红），刻度文字用 `textMuted`。

面板底部是诚实说明：电平跟随系统默认录音设备；通道增益与静音随工程保存；音频尚未混入录制。

数据流：`AudioService` 维护每输入通道的 gain/mute/solo，`MixerPanel` 只发 `gainChanged` / `muteChanged` / `soloChanged` 信号；`MainWindow` 把 gain/mute 写回 `InputDescriptor`（随工程持久化）并调用服务，solo 只作为监听状态存在。独奏时非独奏通道条变暗（`channelStrip[dimmed="true"]`），头部出现 `SOLO n` 胶囊。

真实电平只来自默认录音设备的 `AudioMeter`；没有输入设备时显示 `NO AUDIO INPUT`，不会用动画伪造电平。

## 设置界面

1000×660，最小 880×560。左侧 178 px 导航 + 右侧 `QStackedWidget`，页脚左侧显示版本，右端是主按钮「确定」与次按钮「取消」（不随平台改变位置）。

| 页 | 内容 |
| --- | --- |
| 常规 | 主题卡片 ×3（含实时缩略图与说明）、跟随系统语言 / 简体中文 / English、启动时恢复上次会话 |
| 视频 | 显示模式（横屏 / 竖屏）、画布预设、宽×高、帧率、宽高比；说明画布尺寸决定合成与录制分辨率 |
| 音频 | 输入设备、采样格式、监听说明与当前限制 |
| 输出 | 录制目录（绝对路径）、ffmpeg 可用性、RTMP 服务器与串流密钥 |
| 快捷键 | 键盘与鼠标操作对照（`shortcutKey` 键帽样式） |
| 关于 | 版本、Qt、编译器、平台、工程主页、许可、设计参考 |

交互约定：

- 主题卡片选中即预览：对话框发 `themePreviewRequested`，主窗口立即套用；取消恢复进入时的主题。
- 语言只在接受后写入 `ui/language`，下次启动生效；当前选择存 `ui/language` 的值，留空表示跟随系统。
- 「跟随系统语言」是真实设置：启动时语言为空的路径用 `QLocale::system().language() == QLocale::Chinese` 决定是否加载 `zh_CN`。
- 「启动时恢复上次会话」写入 `ui/restoreSession`；关闭后 `MainWindow` 不再恢复 `project.json`。

## 语言与翻译

- 所有 `tr()` 的 context 必须是 `mixvera::ui::类名`（Qt 运行期按命名空间+类查找）。
- 主题名与描述位于 `theme.cpp`，不是 QObject，用带字面量的 `QCoreApplication::translate("Theme", "…")` 让 lupdate 能提取。
- 文案 448 条，`.ts` 全部完成，`.qm` 经 `translations.qrc` 内嵌，发布时无需额外文件。
- 术语沿用 OBS 中文语言包：硬切 / 淡入淡出 / 划像 / 推流 / 录制 / 调音台；不使用 vMix 机器翻译的「切 / 褪色 / 擦拭 / 流」。

## 截图与验收命令行

界面状态可以完全用命令行复现，这也是截图与回归检查的方式：

| 参数 | 作用 |
| --- | --- |
| `--theme charcoal\|yami\|daylight` | 指定主题（等价 `MIXVERA_THEME`） |
| `--lang zh_CN\|en` | 指定语言（等价 `MIXVERA_LANG`） |
| `--canvas WxH` | 启动时覆盖画布尺寸（如 `1080x1920`） |
| `--mode landscape\|portrait` | 启动时切换画布方向（保留长短边） |
| `--show-mixer` | 启动即展开调音台 |
| `--no-restore` | 忽略上次会话，使用默认工程 |
| `--screenshot <path>` | 延时后保存主窗口截图并退出 |
| `--settings-screenshot <path>` | 打开设置对话框、截图并退出 |
| `--settings-page N` | 配合上一项，选中第 N 个导航页（0 起） |
| `--screenshot-delay <ms>` | 截图前的等待时间（默认 1500） |

`tests/ui/test_ui_smoke.cpp` 覆盖主题：三套样式表非空、无残留 `{{token}}`、互不相同、键别名正确，以及 `mixvera/theme=daylight` 时主窗口样式表确实包含浅色底色。

## OBS / vMix → Mixvera 对照（0.4 增量）

| 设计点 | 证据 | 落点 |
| --- | --- | --- |
| 三套主题、蓝/橙两套强调色 | OBS `Yami.obt` 的 `--grey*` / `--blue3`；vMix 截图取色 | `theme.cpp` |
| 浅色高对比主题 | OBS 自带 Light 主题的分类思路（浅底 + 深字 + 深强调） | `theme.cpp` 的 Daylight |
| 通道条：名称 / 推子 / 电平 / 静音 | OBS 主题对象名 `vMixerScrollArea`、`volMeterFrame` 与混音器语义 | `mixer_panel.cpp` |
| 竖向条与数值刻度 | vMix 调音台形态 | 同上 |
| 设置左侧分类导航 | OBS「设置 → 常规/视频/音频/输出/快捷键/关于」的分类 | `settings_dialog.cpp` |
| 主题选择即时预览 | OBS 外观设置的选择方式 | `SettingsDialog::themePreviewRequested` |
| 主窗体低频按钮、设置承载配置 | vMix 语言包条目分布（主窗体 38 条 vs 设置 262 条） | 主窗口 / 设置的信息分层 |

## 0.5 增量：叠加层、转场与标题（SinSam 观察）

0.5 的功能范围来自 [RE-SINSAM.md](RE-SINSAM.md) 的功能地图；界面上没有引入 SinSam 的任何视觉元素，只借鉴总线顺序与交互语义。

| 设计点 | 证据（SinSam） | 落点 |
| --- | --- | --- |
| 4 路下游叠加层，位于转场后、FTB 前 | tools.json `OverlayInput1..4`、`PreviewOverlayInput*`；vMix 同语义 | `OverlayLayer`、`Compositor::applyOverlays`、页脚 OVERLAY 1–4 |
| 预览叠加开关 | `PreviewOverlayInput` | 页脚 `PVW OVL` 复选框 |
| 每层可指定任意输入、独立开关 | tools.json Overlay 类别 | 页脚按钮（点击开关、右键换输入）与输入条右键「叠加层」子菜单 |
| Fly / Cube 转场 | tools.json Transition 类别的 `Fly`、`Cube` | `TransitionKind` 扩展、`Compositor::mix`、转场栏 2×3 网格 |
| 标题预置：全屏 / 下三分之一 / 倒计时 / 比分牌 | SKILL.md 标题控制与 `TitleProperty*Layout` 资源名 | `TitleProvider` 四种绘制、`AddInputDialog` 标题页 |
| 倒计时开始 / 暂停 / 重置 | tools.json Title 类别 | 输入条右键「标题控制」子菜单；运行参数（end/remaining/paused）随工程保存 |
| 比分递增 / 递减 | tools.json `Increment` | 同一子菜单的「主队/客队得分 ±1」 |
| 截图（含叠加与 FTB 的节目输出） | tools.json Output 类别的 `Snaphost`（原文如此） | 页脚「快照」按钮、输出菜单、`Ctrl+Shift+P` |

叠加层像素流程刻意保持简单：预览与节目先各自 `renderInput`，转场混合后按 1→4 的顺序把每层以 `drawCover` 铺满画布，最后 FTB。录制与推流收到的就是这条链路的输出。

## 0.6 增量：竖屏画布、抠像与字幕（SinSam 观察）

0.6 的范围同样来自 [RE-SINSAM.md](RE-SINSAM.md)。三项能力都接在已有管线上，没有引入新窗口、新依赖或专有格式。

### 画布方向

- `Project::canvas` 是唯一尺寸来源；`MainWindow::setCanvasSize(width, height, persist)` 负责钳制（宽 320–7680、高 180–4320）、写回工程、通知 `Compositor`、刷新监视器，并按需保存。
- `Compositor::sync()` 在输入变化时通过 `SourceProvider::setCanvasSize(QSize)` 把画布尺寸下发给每个 provider；标题据此计算 `unit = 画布宽 / 1280`，字体、内边距、卡片与滚动速度都随画布缩放。
- 「视图 → 竖屏画布」不写死 1080×1920，而是交换当前工程的长短边，因此 720p 工程会得到 720×1280；再次选择恢复横屏。
- 录制、推流与快照直接消费合成器输出，画布一变，输出尺寸随之变化。`SettingsDialog` 的视频页与 `--canvas` 是两个入口；`--canvas` 只在本次运行覆盖，不主动保存，但退出时的会话保存仍会把当前画布写入 `project.json`。

### 蓝 / 绿幕抠像

参数存放在 `InputDescriptor::params`，随工程序列化：

| 参数 | 含义 | 默认 |
| --- | --- | --- |
| `chromaEnabled` | 是否开启 | 关 |
| `chromaColor` | 键色，预设绿 `#00ff00` / 蓝 `#0000ff`，也可任意 | 绿 |
| `chromaSimilarity` | 键色容差，0–100 | 35 |
| `chromaSmoothness` | 边缘羽化，0–100 | 18 |
| `chromaSpill` | 溢色抑制，0–100 | 60 |

- 支持屏幕、窗口、摄像头、图片、视频五类输入；标题、纯色、测试图案与场景不显示抠像入口。
- `Compositor::refresh()` 在缓存帧之后调用 `applyChromaKey()`。算法比较归一化 RGB 色度而不是原始 Cb/Cr：暗部绿幕与亮部色度相同，褶皱和阴影能一起键掉。
  - `threshold = 0.03 + similarity × 0.0055`；距离不超过阈值的像素 alpha 归零。
  - `feather = 0.004 + smoothness × 0.0022`；阈值与阈值 + 羽化之间用 smoothstep 过渡。
  - 绿键 / 蓝键时按 `spill` 把主通道压向另外两个通道的最大值，去掉头发与肩部溢色。
- 输出为预乘 ARGB，场景、叠加层与转场仍然可以直接用 `QPainter` 合成。
- `Compositor::providerParams()` 剔除 `chroma*` 键：调参数不会重建摄像头或采集 provider，画面不闪断。

### 字幕条与滚动字幕

`TitleProvider` 的样式扩展为 `caption` 与 `ticker`：

- **字幕条**：底部安全区（距底 `max(36 × unit, 画布高 4.5%)`）的全宽半透明底板，居中自动换行白字，底板下缘一条强调色线；文字为空时不绘制。
- **滚动字幕**：底部条高 `84 × unit`，文字优先取 `text`，为空时取当前预置句；从右缘进入、按 `tickerSpeed` 像素/秒左移，用 `fmod` 实现无缝循环，条顶一条强调色线。
- `captions` 存多行预置文案，`captionIndex` 指向当前行，`text` 是实际渲染文本。输入条右键「下一句 / 上一句」按行数取模并同时写回 `captionIndex` 与 `text`；「设置字幕文字…」直接改 `text`，`captionClear` 清空。
- 样式、文案、当前行与滚动速度都随工程保存；竖屏下字幕按画布宽度换行，不再被横向工作区裁切。

## 0.6 增量（续）：GT 标题包装与显示模式

0.6 的界面入口只有三处新增：「添加输入」的「标题包装」样式与元素编辑器、输入条右键的标题预置菜单，以及设置 → 视频的「显示模式」。其余变化都在渲染层。

### 显示模式

- 设置 → 视频的画布卡片第一行是 `Display mode`（对象名 `canvasMode`，取值 `landscape` / `portrait`），下面是预设、分辨率与帧率。
- 三个控件互相约束：选竖屏预设会把模式切到竖屏；手改宽高时按 `height > width` 回写模式；方形画布没有方向，切换模式时按 16:9 展开另一条边（1080×1080 → 竖屏 1080×1920，或横屏 1920×1080）。
- 主窗口「视图」菜单提供互斥的「横屏模式 / 竖屏模式」动作（`landscapeAction` / `portraitAction`），与设置、`--mode` 写的是同一份 `Project::canvas`。
- `--mode landscape|portrait` 只覆盖本次运行的画布，不写 QSettings；退出会话保存仍会记录当前画布，与 `--canvas` 一致。

### 标题包装

`TitlePackage` 是只读模型：`load()` 解析一次，`render(canvas, …)` 按任意画布逐帧栅格化。

- **格式**：`.gtzip`（zip 内 `document.xml` + `resources.xml`）用内置只读 ZIP 读取器打开（stored / deflated，拒绝加密、Zip64，单个包上限 64 MB / 2048 项）；`.xaml` / `.sinsamtitle` 与未压缩目录直接读 `document.xml`，图片相对文档目录解析。UTF-8 BOM 与 XML 声明不一致时仍按 UTF-8 解析。
- **缩放**：先 `contain` 到画布，再对 GT Vertical 的 3412×1920 艺术板按高度适配并居中——竖屏 1080×1920 下正好取中间 1080 宽的区域。
- **元素**：容器 `Composition` / `Layer` 递归绘制；`Rectangle`（圆角 / 描边）、`Ellipse`、`Triangle`、`RightTriangle`、`TextBlock`、`Image`、`Ticker`、`QRCode`。容器与叶子都应用 `Opacity`、`Visible`、`Transform Rotate` 与动画状态。
- **画笔**：`Brush Type` 为空时按子节点推断（`Bitmap` 子节点 → 位图，`Stops` / `StartPoint` → 线性渐变）；缺 `Color` 视为透明；色标按 `Position` 稳定排序。
- **动画**：`Storyboard` 按 `Type` 分成入口（空）、`TransitionOut`、`DataChangeIn`、`DataChangeOut`；缺省 `Duration` 0.5 s。`animationFor()` 把 Fade / Fly / Reveal / Expand / ZoomFade / Zoom / Bounce / Rotate 折算为 `AnimationState{opacity, offset, scale, rotation, reveal, revealDirection, scaleAnchor}`，由 `drawNode` 统一应用。
- **时钟**：`titlePackageClockText()` 求值 `{0:...}`；`ClockParts` 解析 .NET 自定义格式（大小写 `H/h`、`m`、`s`、`f/F`、`t/tt`、`d/M/y`、引号 / 转义 / `%`）；倒计时取 `|` 后段，`tt` 为空时去掉前面的空格。
- **二维码**：`qr_encoder.cpp` 独立实现字节模式编码（ECC M、版本 1–10、RS 纠错、格式/版本 BCH、8 种掩码惩罚选择）；渲染 4 模块静区与黑色模块，空内容画红色占位。
- **预置**：输入参数 `titlePreset:<n>:label` 与 `titlePreset:<n>:<fullKey>` 快照全部 `titleText:` / `titleColor:` / `titleImage:`；「下一个 / 上一个」按已有槽位循环，应用前先擦除当前元素覆盖，再写入预置，并记录 `titlePresetChanged`。provider 用该时间戳选择 `DataChangeIn` 并计算 `dataChangeSeconds`。
- **原地更新**：`TitleProvider::update()` 拒绝样式与包装路径变化；其余参数重放后仍保留 `packageClock_`、`tickerClock_` 与 `presetChangedMs_`。`Compositor::sync()` 因此可以在文本 / 颜色 / 图片 / 预置变化时跳过重建，滚动字幕与入场动画不回到起点。

## 边界

- 调音台不做音频路由与混音总线；solo 只是监听状态，不改变输出。
- 没有为高 DPI 单独提供图标位图；SVG 图标由 Qt 缩放。
- Daylight 主题下监视器仍为近黑底，这是刻意的：节目画面不应因为界面主题变亮而失真。
- 叠加层是全画布 `drawCover`，不做逐层位置/缩放调整；需要布局时把内容放进场景再作为叠加输入。
- 倒计时走真实时钟（`countdownEnd` 时间戳），暂停/恢复会换算剩余毫秒；GT 包装的倒计时字段使用 `|` 后的格式段。
- 抠像是整幅色度键控，不做 SinSam 的区域抠图、遮罩形状（圆形 / 矩形 / 爱心等）或 AI 抠像；光线复杂时靠相似度 / 平滑度 / 溢色三个参数手动调。
- 标题包装只读：支持 GT 文档的常用子集（元素、画笔、Storyboard、Ticker、时钟、QR），不渲染 `Effect`（模糊 / 阴影）、`DataFlags` 数据绑定，也不读取 `.pag` / `.vzae` 特效字幕、`.mov` 透明动画或在线标题；Mixvera 不是 GT 设计器，不写回 `.gtzip`。
- 包装文本按 Qt 字体度量排版；GT 模板引用的字体缺失时回退到系统字体链，换行与字距可能与 vMix / WPF 略有差异。
- 竖屏只改变画布方向，工作区仍是横向双监视器；SinSam 的「切换界面模式 / 切换界面布局」与「专业模式 / 电商模式」不移植。
- 不复制 OBS / vMix / SinSam 的 QSS 全文、图标、字体、语言包文本、窗体坐标或标题模板。
