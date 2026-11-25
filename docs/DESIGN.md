# Mixvera 设计说明（0.4）

这份文档说明 0.4 的界面、主题、调音台与设置界面的设计依据和实现约定。配色与控件语义的取证过程见 [UI-SPEC.md](UI-SPEC.md)，两份原程序的原工程结构复原见 [RE-PROJECTS.md](RE-PROJECTS.md)。

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
| 转场列 | 固定 156 | 硬切 / 自动 / 四种转场 / FTB / T-BAR / 时长 |
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
| 视频 | 画布预设、宽×高、帧率、宽高比；说明画布尺寸决定合成与录制分辨率 |
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
- 文案 305 条，`.ts` 全部完成，`.qm` 经 `translations.qrc` 内嵌，发布时无需额外文件。
- 术语沿用 OBS 中文语言包：硬切 / 淡入淡出 / 划像 / 推流 / 录制 / 调音台；不使用 vMix 机器翻译的「切 / 褪色 / 擦拭 / 流」。

## 截图与验收命令行

界面状态可以完全用命令行复现，这也是截图与回归检查的方式：

| 参数 | 作用 |
| --- | --- |
| `--theme charcoal\|yami\|daylight` | 指定主题（等价 `MIXVERA_THEME`） |
| `--lang zh_CN\|en` | 指定语言（等价 `MIXVERA_LANG`） |
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

## 边界

- 调音台不做音频路由与混音总线；solo 只是监听状态，不改变输出。
- 没有为高 DPI 单独提供图标位图；SVG 图标由 Qt 缩放。
- Daylight 主题下监视器仍为近黑底，这是刻意的：节目画面不应因为界面主题变亮而失真。
- 不复制 OBS / vMix 的 QSS 全文、图标、字体、语言包文本或窗体坐标。
