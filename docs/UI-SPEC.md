# Mixvera 界面参照：OBS、vMix 与 SinSam 的可验证观察

这份文档记录 `0.3.0` 布局，`0.4.0` 主题、调音台与设置界面，以及 `0.5.0`–`0.6.0` SinSam / vMix GT 移植的证据来源。原则是**只借鉴公开可观察的界面组织与配色值，不复制任何专有代码、图标或资源**。实现约定（token 表、尺寸、交互）另见 [DESIGN.md](DESIGN.md)。

## 取证环境

| 软件 | 位置 | 用途 |
| --- | --- | --- |
| OBS Studio 32.1.2（本机安装） | `C:\Program Files\obs-studio` | 主题与语言包的对照版本 |
| OBS Studio 33.0.3（安装包） | `D:\Mixvera\OBS-Studio-33.0.3-Windows-x64-Installer.exe` | 从中只读提取 33.0.3 的 `Yami.obt`、`en-US.ini`、`zh-CN.ini`，作为 0.3 的术语与配色主证据 |
| vMix 28 | `C:\Program Files (x86)\vMix` | 语言包、窗体控件清单、程序集元数据 |
| vMix 29 安装包 | `D:\Mixvera\vmix29.exe` | Inno Setup 只读解包，用于核对 29 版新增控件与中文条目；主界面证据以已安装的 vMix 28 与官方截图为底 |
| vMix 28 官方帮助主窗口截图 | `build/analysis/vmix/NewItem446.png`（1245×743，本机分析目录，未入库） | 布局与配色的视觉证据；安装版受授权限制无法启动 |
| vMix 28 安装目录 `titles/` | `C:\Program Files (x86)\vMix\titles` | GT 标题模板库：743 个 `.gtzip` / 87 个 `.xaml`，0.6 只读解析 |

vMix 28 启动后会停在 “License Request — vMix software won't run without a valid license file”，确认没有可用授权，因此**没有运行中的 vMix 界面截图**。布局证据来自官方帮助页截图，取色使用本地像素采样；这条限制不影响控件清单与术语取证。

### 工具与证据

| 观察 | 工具 | 证据 |
| --- | --- | --- |
| OBS 安装包为 32 位 PE / NSIS，含 89 个函数、1127 个字符串 | craft-anything `core_target_triage` / `core_overview_show` | `ev_327fef0c…afd5b6`、`ev_91b345c1…3f8ab` |
| 直接从安装包提取的 `Yami.obt`，SHA-256 `fb778672…6a702` | craft-anything `core_text_read` | `ev_0d357aeb…b663d` |
| vMix 29 安装包为 32 位 PE / Inno Setup，入口 `0x4b5eec` | craft-anything `core_target_triage` | `ev_8792cf0a…d32045` |
| 已安装 vMix 28 为 PE32 / .NET Framework 4.0 / IL-only，程序集 `28.0.0.42` | REA `inspect_managed_artifact` | `ev_6829731f…c910d`（父证据 `ev_72461980…f6e9e`） |

完整的原工程结构复原（OBS 的 CMake / Qt / libobs / 插件树，vMix 的 VB.NET 模块、窗体与资源清单）见 [RE-PROJECTS.md](RE-PROJECTS.md)。

安装包 SHA-256：

- OBS：`700be33c01468c2a1e43820bbcc49ba914f570f0f82250022bb680fba43b2a67`
- vMix 29：`48624286f53fcbbcfd1ce7a0c81f8bf8ac2bb2be6a6e68ffcc1d9da86a00e467`

### 0.3 补充取证

- **OBS 版本校正**：本机安装是 32.1.2（`obs64.exe` FileVersion 32.1.2），不是 33.0.3。33.0.3 的文件是从安装包只读提取的：`zh-CN.ini` 90,063 B / 1,410 行，`Yami.obt` 67,936 B / 2,887 行；安装版 32.1.2 只作对照。
- **vMix 28 主窗体**：REA `inspect_managed_members` 在 `vMix.frmmain` 上恢复出 186 个字段、515 个方法和 171 个原始 VB 控件名；`pMain` / `pPreview` 分别是节目与预览监视器。`vMix.exe.config` 的默认值是 `Theme=Charcoal8`、`PreviewColour=DarkOrange`、`ActiveColour=DarkGreen`、`InputSize=208x156`，音频混音器默认停靠但隐藏——与本项目「橙色预览 / 绿色节目 / 输入条 / 可收起调音台」一致。
- **vMix 29 安装包**：Inno Setup 6.1.0 Unicode，481,764,296 B；用 `innoextract 1.9` 只读解出 `app/languages/zh.txt`（95,288 B，1,297 条 / 267 个窗体）和 `vMix.exe`（29.0.0.50）。安装器本身只有英文，没有运行安装器。
- **vMix 中文质量**：`zh.txt` 自己标注 “Translations provided by Google Translate”；`Crop=庄稼`、`Pan X=潘贤章`、`T-Bar=T-酒吧`、`Output=产量` 等错误说明它只能作为控件/术语清单，不能作为发布文案。

对应证据：`ev_5ae1905a…bf64c`（frmmain 摘要）、`ev_a9b312b4…3e1e8`（frmmain 成员）、`ev_b7ac29d9…891be`（vMix 28 `zh.txt`）、`ev_ad36d9bd…20c6`（OBS 33.0.3 `zh-CN.ini`）。

**分析限制**：本机未配置 Ghidra 12.1 / IDA MCP，REA 的原生反编译工具不可用；vMix 28 的 `InitializeComponent` 被 Eziriz .NET Reactor 保护成 4/6 字节桩，因此窗体的精确坐标、字体和控件尺寸无法静态恢复。`vmix29.exe` 超过 REA 的 64 MB PE 资源与 128 MB 托管快照上限，29 版只做了 innoextract 静态解包；两份安装器都没有执行。布局坐标以官方帮助截图为准，控件清单以语言包与托管元数据为准。

## OBS：配色与结构

`data/obs-studio/themes/Yami.obt` 是纯文本（`@OBSThemeMeta` + `@OBSThemeVars` + QSS），可直接读取。取到的关键颜色：

| Token | 值 | 在本项目中的用途 |
| --- | --- | --- |
| `--grey8` | `#13141A` | 监视器背景、列表底 |
| `--grey7` | `#1D1F26` | 主窗口背景 |
| `--grey6` | `#272A33` | 停靠面板 |
| `--grey5` | `#323540` | 控件底色 / 分隔线 |
| `--grey4` | `#3C404D` | 边框、描边 |
| `--blue3` | `#284CB8` | 选中、主按钮、AUTO |
| `--blue2` | `#476BD7` | 悬停、焦点框 |
| `--red3` | `#C01C37` | 录制 / PGM / 危险态 |
| `--green2` | `#37D247` | PVW、正常电平 |
| `--yellow2` | `#E5AF24` | 电平预警 |
| `--white2` | `#E1E1E1` | 正文 |
| `--white5` | `#969696` | 次要文字 |

主题 QSS 里可观察到的控件对象名包括：`previewScalingMode`、`previewZoomInButton`、`previewZoomOutButton`、`previewScalePercent`、`stackedMixerArea`、`vMixerScrollArea`、`hVolumeWidgets`、`vVolumeWidgets`、`volMeterFrame`、`streamButton`、`recordButton`、`replayBufferButton`、`broadcastButton`、`virtualCamButton`、`modeSwitch`、`controlsFrame`。这些名字说明 OBS 的主界面由「预览缩放、混音器、录制/推流按钮、模式切换、控制区」构成。

语言包 `data/obs-studio/locale/en-US.ini` 给出了界面语义：

- `Basic.Main.Scenes="Scenes"`、`Basic.Main.Sources="Sources"`、`Basic.Main.Mixer="Audio Mixer"`、`Basic.Main.Controls="Controls"`
- `Basic.Main.StartRecording` / `StopRecording` / `StartStreaming` / `StopStreaming`
- `Basic.Main.Sources.Visibility` / `Lock`（源列表的显隐与锁定）
- `Basic.Main.PreviewDisabled="Preview is currently disabled"`

**本项目采用**：深色三级表面（窗口 / 面板 / 监视器）、单一蓝色强调色、红色只用于 PGM 与录制、左侧「Scenes / Sources」两栏、底部音频混音器、源列表带显隐勾选。

## vMix：主窗口与输入

`vMix.exe` 经 REA 托管程序集检查：PE32、CLI、`.NET Framework 4.0`、IL-only、程序集版本 `28.0.0.42`（sha256 `7b9168ae…31d8`）。界面控件清单来自 `languages/en.txt`，格式为 `窗体.控件<TAB>英文<TAB>英文`，可直接还原出主窗口的操作集合。

`frmmain.*` 中的关键控件：

| 控件 | 文案 | 含义 |
| --- | --- | --- |
| `cmdCut` | Cut | 硬切 |
| `cmdFade` | Fade | 淡入淡出转场 |
| `cmdFade2` | Wipe | 划像 |
| `cmdFade4` | Slide | 滑动 |
| `cmdEffect` | Zoom | 缩放 |
| `cmdFTB` | FTB | 淡出到黑 |
| `cmdAddInput` | Add Input | 添加输入 |
| `cmdOverlaySettings` | Overlay | 叠加层 |
| `cmdRecStartStop` | Record | 录制启停 |
| `cmdStreaming` | Stream | 推流启停 |
| `pctRec` / `pctStream` / `pctSRT` / `pctExt` / `pctSolo` | REC / STREAM 1 / SRT / EXT / SOLO | 状态指示 |
| `cmdAudioMixer` | Audio Mixer | 混音器 |
| `cmdPlaylist` / `cmdReplay` / `cmdMultiCorderStartStop` | PlayList / Instant Replay / MultiCorder | 附加功能 |
| `cmdSettings` / `cmdPause` / `cmdAdvanced` | Settings / Pause Inputs / Advanced | 设置与状态 |
| `cmdOpen` / `cmdSave` / `cmdSaveExisting` / `cmdClose` / `cmdLastPreset` | Open / Save As / Save / New / Last | 预设管理 |

输入对话框 `frmInput.*` 的输入类型分组：Audio（`tAudio`）、Image（`tImage`）、Photos（`tPhotos`）、PowerPoint（`tPowerpoint`）、DVD（`tDVD`）、Capture（`tCapture`）、Desktop/Network（`tNetwork`）、NDI（`tNDI`）、Title（`tTitle`）、Colour（`tColour`）、Video List（`tVideoList`）、Virtual Set（`tVirtualSet`）、RTSP（`tRTSP`）、Flash（`tFlash`）、Video Delay（`tVideoDelay`）、Audio Input（`tAudioInput`）。

窗体分布（语言包条目计数）也印证了 vMix 的「专业密度」：设置窗体 262 条、输入设置 134 条、输入对话框 130 条、音频设置 63 条，而主窗体只有 38 条 —— 主界面刻意保持少量高频按钮。

**0.2 曾采用**：PGM 大、PVW 小的双监视器；右侧输入缩略图列表。**0.3 起按官方帮助截图重排**，见下一节。

## vMix 主窗口布局与配色（0.3）

官方帮助截图里可以直接观察到的主窗口结构：

- 顶部菜单与工具栏；右端是 `REC` / `STREAM` 等状态区
- 主工作区从左到右：**Preview 监视器**（橙色标题条）→ **竖向转场控制列** → **Program 监视器**（绿色标题条）
- 转场列自上而下：Quick Play、`Cut`、`Fade / Merge / Wipe / CubeZoom` 等类型、`FTB`、转场时长 1–4、竖向 `T-Bar`
- 底部**横向输入条**：每块输入带缩略图与标题条，PGM 绿、PVW 橙；上方有分类标签与搜索框，右端是 `Audio Mixer` 入口
- 输入条下方是功能栏：`Add Input`、`Record`、`External`、`Stream`、`MultiCorder`、`PlayList`、`Overlay`
- 最底部是状态栏

对截图取样的主要颜色：

| 区域 | 颜色 |
| --- | --- |
| 主窗口背景 | `#1E2328` |
| 转场面板 | `#404B56` |
| 输入条 / 页脚 | `#293038` |
| 状态栏 | `#0A0F14` |
| Preview 标题条 | `#FF8C00` |
| Program / PGM 标题条 | `#006400` |
| 分类标签（紫 / 深蓝） | `#800080` / `#191970` |

**0.3 采用**：Preview 左、Program 右，中央 156 px 竖向转场列；底部横向输入条 + 页脚；场景/来源与调音台改为可收起的侧栏；顶栏保留品牌、工程名、面板开关、设置、全屏、REC/STREAM。上面的采样值写入 `theme.cpp` 与 `stage_view.cpp`，没有使用 vMix 的图标或布局坐标。

## 0.4：主题、调音台与设置

0.4 不新增截图证据，而是把已有证据落到可切换的主题与设置结构上。

### 主题

| 主题 | 证据 | 采用方式 |
| --- | --- | --- |
| Charcoal | vMix 主窗口采样值 `#1E2328` / `#404B56` / `#FF8C00` / `#006400` | 作为默认主题的窗口、转场面板与信号色 |
| Yami | OBS `Yami.obt` 的 `--grey8 #13141A`、`--grey7 #1D1F26`、`--grey6 #272A33`、`--blue3 #284CB8`、`--red3 #C01C37`、`--green2 #37D247`、`--yellow2 #E5AF24` | 直接的 token 映射：`window` / `surface` / `raised` / `accent` / `danger` / `success` / `warning` |
| Daylight | OBS 官方同时提供深色与浅色主题这一事实 | 按同样信息层级设计浅色值（浅底 `#EEF1F5`、深字 `#1B2430`、深橙强调 `#C2410C`）；没有照搬 OBS 浅色主题的 QSS |

三套主题共享同一份 QSS 模板和同一组 47 个 token；预览橙、节目绿、录制红作为**功能信号色**在每套主题里都保留（Daylight 使用同色相略深的变体以满足白底对比度）。

### 调音台

OBS 的主题对象名把混音器拆成 `vMixerScrollArea`（可滚动通道区）、`hVolumeWidgets` / `vVolumeWidgets`（横/竖布局）、`volMeterFrame`（电平框），vMix 的调音台则是竖向条。Mixvera 取两者的交集：

- 竖条、可横向滚动 —— vMix 形态
- 条内自上而下为编号、名称、dB 数值、推子、`M`/`S`，主通道单独一条带刻度电平 —— OBS 通道条的信息顺序
- 主表分段取 OBS `--green2` / `--yellow2` / `--red3` 的语义：正常绿、预警黄、削波红

没有照搬 OBS 的混音器布局坐标或控件尺寸。

### 设置

vMix 语言包的条目分布显示主窗体只有 38 条、设置窗体 262 条，即「主界面少量高频按钮、配置集中在设置」。OBS 的设置分类（常规 / 视频 / 音频 / 输出 / 快捷键 / 关于）被直接采用为左侧导航的六页；主题卡片与即时预览对应 OBS 外观设置里选择主题即可看到效果的交互。

### 未采用

- 不复制 `Yami.obt` 的 QSS 规则或变量命名，只取颜色值
- 不复制 OBS 的设置项全文（输出模式、编码器矩阵等），只取分类结构
- 不复制 vMix 的调音台按钮排布与图标

## 中文术语

vMix 的 `languages/zh.txt` 存在明显的机器翻译痕迹，例如 `Cut → 切`、`Fade → 褪色`、`Wipe → 擦拭`、`Record → 记录`、`Stream → 流`。这些词在中文导播语境里不自然，也不利于现场操作。

OBS 33.0.3 的 `locale/zh-CN.ini` 使用了更专业的说法：`剪切`、`淡入淡出`、`划像`、`录制`、`推流`、`场景`、`源`、`混音器`。OBS 把 Program 译作「输出」，而现场制作更常用「节目」；Mixvera 采用后者，与 vMix 的 PGM/PVW 语义一致。

Mixvera 的取舍：

| 英文 | vMix zh.txt | 本项目 |
| --- | --- | --- |
| Cut | 切 | 硬切 |
| Fade | 褪色 | 淡入淡出 |
| Wipe | 擦拭 | 划像 |
| Slide | 滑动 | 滑动 |
| Zoom | 缩放 | 缩放 |
| Record | 记录 | 录制 |
| Stream | 流 | 推流 |
| Preview | 预览 | 预览 |
| Program | 节目 | 节目 |
| Audio Mixer | 音频混合器 | 调音台 |

「硬切」强调 Cut 的无过渡特性；「调音台」是现场制作里比「音频混合器」更常用的叫法。英文界面仍保留 vMix 的 `CUT` / `AUTO` / `FTB` / `T-BAR` 缩写，便于对照。

## 对照表

| 界面元素 | 来源 | Mixvera 落点 |
| --- | --- | --- |
| vMix 主窗口配色与三联布局 | vMix 帮助截图 | `src/mixvera/ui/theme.cpp`、`main_window.cpp` |
| Preview 左 / 转场列中 / Program 右 | vMix | `src/mixvera/ui/main_window.cpp` |
| 橙色预览条 / 绿色节目条 | vMix | `src/mixvera/ui/stage_view.cpp` |
| 竖向转场列（Cut / Auto / 四种转场 / FTB / T-Bar） | vMix | `src/mixvera/ui/transition_bar.cpp` |
| 横向输入条 + 缩略图 + 编号 | vMix | `src/mixvera/ui/input_panel.cpp` |
| 底部功能栏（添加输入 / 录制 / 推流） | vMix | `main_window.cpp` 页脚 |
| 可收起场景 / 来源栏 | OBS | `src/mixvera/ui/scene_panel.cpp` |
| 源显隐勾选、上移/下移 | OBS | 同上（`sourceList`） |
| 右侧调音台 + 电平表 | OBS + vMix | `src/mixvera/ui/mixer_panel.cpp` |
| 三套主题与 token 化的 QSS | OBS + vMix | `src/mixvera/ui/theme.cpp`、`theme_stylesheet.hpp` |
| 设置左侧分类导航与主题即时预览 | OBS | `src/mixvera/ui/settings_dialog.cpp` |
| 录制 / 推流状态胶囊 | OBS + vMix | `main_window.cpp` 顶栏 `pillRec` / `pillStream` |
| 中文术语 | OBS zh-CN | `translations/mixvera_zh_CN.ts` |
| 语言加载与回退 | Qt i18n | `apps/mixvera/main.cpp`、`settings_dialog.cpp` |

## 0.5：SinSam（芯象）叠加层与标题

0.5 的证据来自对 `sinsam_setup.26.7.10.3.exe` 的只读逆向，完整记录见 [RE-SINSAM.md](RE-SINSAM.md)。只取总线顺序与交互语义，不取 SinSam 的任何视觉资源：

| 界面元素 | 来源 | Mixvera 落点 |
| --- | --- | --- |
| 页脚叠加层行（OVERLAY 1–4，点击开关 / 右键选输入） | SinSam tools.json `OverlayInput1..4`；vMix 同语义 | `main_window.cpp` 页脚、`OverlayLayer` |
| 预览叠加复选框 | SinSam `PreviewOverlayInput*` | 页脚 `PVW OVL`（`previewOverlaysToggle`） |
| 输入条右键「叠加层」子菜单 | SinSam Overlay 工具类别 | `input_panel.cpp` `showContextMenu` |
| Fly / Cube 转场按钮（2×3 网格） | SinSam tools.json Transition `Fly`、`Cube` | `transition_bar.cpp`、`compositor.cpp` `mix()` |
| 标题四样式：全屏 / 下三分之一 / 倒计时 / 比分牌 | SinSam 标题预置与 `TitleProperty*Layout` 资源名 | `source_providers.cpp` `TitleProvider`、`add_input_dialog.cpp` |
| 标题控制子菜单（倒计时开始/暂停/重置、比分 ±1） | SinSam tools.json Title 类别与 `Increment` | `input_panel.cpp`、`main_window.cpp` `onTitleAction` |
| 快照按钮与 `Ctrl+Shift+P` | SinSam Output 类别 `Snaphost`（原文如此） | `main_window.cpp` `onSnapshot` |

## 0.6：SinSam 竖屏、抠像与字幕

0.6 的证据来自同一安装包的 `sinsam.dll`：UTF-8 字符串块给出中文功能名，托管元数据给出对应的参数类型与设置方法。偏移是 `sinsam.dll` 文件内的字节偏移，用于复现；没有反编译方法体。

| 界面元素 | 证据（`sinsam.dll` 偏移 / 托管名） | Mixvera 落点 |
| --- | --- | --- |
| 竖屏 / 横屏画布 | `竖屏功能` `0x874c3b`、`横屏` `0x876d79`、`竖屏` `0x876d85`、导入尺寸提示 `1080*1920` `0x8769df`；许可证标签 `最大分辨率` `0x874c26` | 画布预设（1080×1920 / 720×1280 / 1080×1080）、「视图 → 竖屏画布」、`--canvas` |
| 界面模式切换 | `切换界面模式` `0x875e3b`、`切换界面布局` `0x875df3`、`专业模式` `0x876d55`、`电商模式` `0x876d67` | **未采用**：工作区仍为横向双监视器，只改变画布方向 |
| 蓝绿专用 / 任意颜色抠图 | `蓝绿专用抠图` `0x87537e`、`任意颜色抠图` `0x875396`、`高级抠图` `0x8753ae` / `0x8775d7` | 输入属性「CHROMA KEY」：绿幕 / 蓝幕 / 自定义色 |
| 抠像参数分组 | `VZ_ChromaKeyParam` `0x78a72f`、`VZ_KeyCleanerParam` `0x78a741`、`ChromaKeyMatting` `0x78d85c`、`ChromaKeyMattingColorType` `0x78d88e`、`SetChromaKeyMattingParam` `0x7ee2d8`、`SetKeyCleanerParam` `0x7ec367` | `chromaColor` / `chromaSimilarity` / `chromaSmoothness` / `chromaSpill`；键色 + 容差 + 边缘 + 溢色四段 |
| 区域抠图 / 遮罩 | `区域抠图改变` `0x875c1f`、`抠图初始化` `0x875c37`、`VZ_ChromaKeyAreaParam` `0x78a719` / `0x7f9ee8`、`SetChromaKeyMask` `0x7ee3bb`、`FrmChromaKeyMaskPoint` `0x7900ed` / `0x845d74` | **未采用**：不做区域限定与异形遮罩，保持整幅键控 |
| 本地字幕 / 在线字幕 | `本地字幕` `0x874cc2`、`在线字幕` `0x874cd4` | 字幕条与滚动字幕；在线字幕与 AI 语音不移植 |
| 字幕编辑与文案 | `字幕编辑` `0x87180b`、`字幕文案` `0x87181d`、`字幕设计编辑器` `0x874e67` | 多行预置文案 + 输入条右键逐句切换；不做设计器 |
| 滚动字幕 | `TitleTicker` `0x78d7b0` | 标题「滚动字幕」样式，20–800 px/s |
| 语音字幕 | `TitleSpeech` `0x78d7a4`、`AISpeechSource` `0x79bbc2` | **未采用**：依赖在线语音服务 |

字幕与抠像都只取「功能名 → 参数语义」这一层：Mixvera 的文字排版、底板颜色、圆角、滚动条形态全部自绘，没有在仓库中分发或复制 SinSam 的 `.gtzip` / `.sinsamtitle` / `.pag` 模板，也没有复制其界面截图或字体；0.6 起只读解析 GT 文档（见下一节）。

## 0.6（续）：GT 标题包装与显示模式

0.6 有两个证据面：vMix 28 安装目录的 GT 模板库（只读解析）与 SinSam 的字幕导入字符串。它们决定了「标题包装」样式的控件与可编辑元素。

### vMix GT 模板库

本机 `C:\Program Files (x86)\vMix\titles` 下的 743 个 `.gtzip` 与 87 个 `.xaml`，只读扫描（不运行 vMix）：

| 观察 | 数量 / 取值 | 用途 |
| --- | --- | --- |
| GT Title | 400 | 主模板集，Headline / Description / Top Text 等元素命名 |
| GT Vertical | 147 | 3412×1920 艺术板，中心 1080×1920 画面 |
| GT Stingers / Social | 66 / 63 | 动画与社交模板，Storyboard 覆盖 |
| GT ScoreBoard / Text / Timer / Ticker | 18 / 18 / 17 / 14 | 比分、纯文本、时钟与滚动字幕 |
| `Transform Rotate` | `"x,y,z"`，0–6.28 为主 | 按弧度解析 |
| `Storyboard Type` | 空 / `TransitionOut` / `DataChangeIn` / `DataChangeOut` | 入口、出场、数据变更 |
| `Ticker Direction` | 空为主，少量 `Top` | 竖向滚动 |
| 图片 | `Stretch="Fill"`、`SizeMode="Stretch"` | 铺满目标框 |
| Timer 文本 | `{0:HH:mm}`、`{0:hh:mm tt}`、`{0:hh:mm:ss tt}`、`{0:4:00PM\|mm:ss}` | .NET 时钟格式 |

**界面落点**：添加输入 → 标题 → 「标题包装」行（路径 + `Browse…` + `Elements…`）；包装信息标签显示可编辑元素数量；包装样式下「字幕文案」多行框用作逐句文本。输入条右键新增「保存标题预置…」「上一个 / 下一个标题预置」「删除标题预置」「设置标题文字…」「上一句字幕 / 下一句字幕 / 清除标题文字」。

### SinSam 字幕导入

| 观察（`sinsam.dll`） | 落点 |
| --- | --- |
| 导入过滤器 `.xaml;.gtzip`，描述里另有 `.sinsamtitle`（`0x87678a`） | 文件对话框接受 `*.gtzip *.sinsamtitle *.xaml` |
| 导入提示 `字幕文件格式：gt，gtzip`、100 MB 上限（`0x8767d1`） | 只读文件，不写回；解析失败在对话框内提示 |
| `选择字幕选择预设文本` `0x87572c` | 「保存 / 上一个 / 下一个 / 删除标题预置」命名快照 |
| `字幕元素属性改变` `0x875750`、MCP `GetTitleElementProps` | 「Elements…」列出文本 / 图片 / 颜色元素并可覆盖 |
| `TitleTicker` `0x78d7b0`、`TitleAnimationTickerForm` 资源 | 包装 Ticker 元素与内置滚动字幕共用渲染语义 |

### 显示模式

| 界面元素 | 证据 | 落点 |
| --- | --- | --- |
| 设置 → 视频「显示模式」下拉 | SinSam `横屏` `0x876d79` / `竖屏` `0x876d85`；vMix 输出同样由宽高驱动 | `settings_dialog.cpp` 的 `canvasMode` |
| 视图菜单互斥的横屏 / 竖屏 | 同一组字符串 | `main_window.cpp` 的 `landscapeAction` / `portraitAction` |
| `--mode landscape\|portrait` | 无直接证据，是截图与自动化入口 | `apps/mixvera/main.cpp` |

包装文件只作为输入读取，仓库不分发任何 vMix / SinSam 模板；界面上的路径、元素列表与预置菜单均为自绘。

## 有意不做的事

- 不复制 OBS 的图标、QSS 全文、位图资源；只取颜色值与控件语义
- 不复制 vMix 的窗体布局坐标、业务逻辑或语言包文本；控件名仅用于确认功能集合
- 不实现 vMix 的虚拟集、Photos、PowerPoint、DVD、SRT、MultiCorder 等专有输入与功能
- 不实现 OBS 的虚拟摄像头、Replay Buffer、场景集合（Scene Collection）等本版范围外能力
- 不复制 GT 模板文件及其字体 / 纹理；只读解析 `document.xml` 的语义，仓库附带的演示包装是自绘的

## 复现取证

```powershell
# 从随仓库安装包提取主题与语言文件到已被 git 忽略的分析目录
& 'C:\Program Files\Bandizip\bz.exe' x -y -o:build\analysis\obs `
  'D:\Mixvera\OBS-Studio-33.0.3-Windows-x64-Installer.exe' `
  'data\obs-studio\themes\Yami.obt' 'data\obs-studio\locale\en-US.ini' `
  'data\obs-studio\locale\zh-CN.ini'

# 配色 token（craft-anything core_text_read 亦可读取同一文件）
Select-String -Path build\analysis\obs\data\obs-studio\themes\Yami.obt -Pattern '^    --(blue3|red3|green2|grey[4-8])'

# OBS 界面语义与中文术语
Select-String -Path build\analysis\obs\data\obs-studio\locale\en-US.ini -Pattern '^Basic\.Main\.'
Select-String -Path build\analysis\obs\data\obs-studio\locale\zh-CN.ini -Pattern '^Basic\.Main\.'

# vMix 主窗口控件与中文条目
Get-Content 'C:\Program Files (x86)\vMix\languages\en.txt' -Encoding UTF8 | Where-Object { $_ -like 'frmmain.*' }
Get-Content 'C:\Program Files (x86)\vMix\languages\zh.txt' -Encoding UTF8 | Where-Object { $_ -like 'frmmain.*' }

# vMix 帮助截图主要颜色（Python + Pillow，截图路径为本机分析目录）
python -c "from PIL import Image; from collections import Counter; im=Image.open(r'build\analysis\vmix\NewItem446.png').convert('RGB'); print(Counter(im.getdata()).most_common(12))"
```

`vmix29.exe` 是 Inno Setup 安装包，当前仓库没有内置解包器；不要为了取证直接运行安装器。需要 vMix 29 的控件变化时，先在隔离环境安装，再对安装目录中的 `languages/en.txt` 与 `vMix.exe` 做同样的只读检查。

vMix 程序集元数据通过 REA 的 `inspect_managed_artifact` 获得（证据 id `ev_72461980d1d0479e4c1419b2c29e5ecde420adc2933af8ef932735db1d2f6e9e`）。