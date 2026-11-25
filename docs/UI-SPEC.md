# Mixvera 界面参照：OBS 与 vMix 的可验证观察

这份文档记录 `0.3.0` 界面与中文术语的证据来源。原则是**只借鉴公开可观察的界面组织与配色值，不复制任何专有代码、图标或资源**。

## 取证环境

| 软件 | 位置 | 用途 |
| --- | --- | --- |
| OBS Studio 32.1.2（本机安装） | `C:\Program Files\obs-studio` | 主题与语言包的对照版本 |
| OBS Studio 33.0.3（安装包） | `D:\Mixvera\OBS-Studio-33.0.3-Windows-x64-Installer.exe` | 从中只读提取 33.0.3 的 `Yami.obt`、`en-US.ini`、`zh-CN.ini`，作为 0.3 的术语与配色主证据 |
| vMix 28 | `C:\Program Files (x86)\vMix` | 语言包、窗体控件清单、程序集元数据 |
| vMix 29 安装包 | `D:\Mixvera\vmix29.exe` | Inno Setup 只读解包，用于核对 29 版新增控件与中文条目；主界面证据以已安装的 vMix 28 与官方截图为底 |
| vMix 28 官方帮助主窗口截图 | `build/analysis/vmix/NewItem446.png`（1245×743，本机分析目录，未入库） | 布局与配色的视觉证据；安装版受授权限制无法启动 |

vMix 28 启动后会停在 “License Request — vMix software won't run without a valid license file”，确认没有可用授权，因此**没有运行中的 vMix 界面截图**。布局证据来自官方帮助页截图，取色使用本地像素采样；这条限制不影响控件清单与术语取证。

### 工具与证据

| 观察 | 工具 | 证据 |
| --- | --- | --- |
| OBS 安装包为 32 位 PE / NSIS，含 89 个函数、1127 个字符串 | craft-anything `core_target_triage` / `core_overview_show` | `ev_327fef0c…afd5b6`、`ev_91b345c1…3f8ab` |
| 直接从安装包提取的 `Yami.obt`，SHA-256 `fb778672…6a702` | craft-anything `core_text_read` | `ev_0d357aeb…b663d` |
| vMix 29 安装包为 32 位 PE / Inno Setup，入口 `0x4b5eec` | craft-anything `core_target_triage` | `ev_8792cf0a…d32045` |
| 已安装 vMix 28 为 PE32 / .NET Framework 4.0 / IL-only，程序集 `28.0.0.42` | REA `inspect_managed_artifact` | `ev_6829731f…c910d`（父证据 `ev_72461980…f6e9e`） |

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
| 录制 / 推流状态胶囊 | OBS + vMix | `main_window.cpp` 顶栏 `pillRec` / `pillStream` |
| 中文术语 | OBS zh-CN | `translations/mixvera_zh_CN.ts` |
| 语言加载与回退 | Qt i18n | `apps/mixvera/main.cpp`、`settings_dialog.cpp` |

## 有意不做的事

- 不复制 OBS 的图标、QSS 全文、位图资源；只取颜色值与控件语义
- 不复制 vMix 的窗体布局坐标、业务逻辑或语言包文本；控件名仅用于确认功能集合
- 不实现 vMix 的虚拟集、Photos、PowerPoint、DVD、SRT、MultiCorder 等专有输入与功能
- 不实现 OBS 的虚拟摄像头、Replay Buffer、场景集合（Scene Collection）等本版范围外能力

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