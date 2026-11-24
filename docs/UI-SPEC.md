# Mixvera 界面参照：OBS 与 vMix 的可验证观察

这份文档记录 `0.2.0` 界面设计的证据来源。原则是**只借鉴公开可观察的界面组织与配色值，不复制任何专有代码、图标或资源**。

## 取证环境

| 软件 | 位置 | 用途 |
| --- | --- | --- |
| OBS Studio 33.0.3 | `C:\Program Files\obs-studio` | 主题文件、语言包 |
| vMix 28 | `C:\Program Files (x86)\vMix` | 语言包、窗体控件清单、程序集元数据 |
| OBS 33.0.3 安装包 | `D:\Mixvera\OBS-Studio-33.0.3-Windows-x64-Installer.exe` | 从安装包内重新提取 `Yami.obt` 与语言包，核对本机安装版本 |
| vMix 29 安装包 | `D:\Mixvera\vmix29.exe` | 识别安装器类型与版本线索；主界面控件清单仍以已安装的 vMix 28 为准 |

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

**REA 深度分析限制**：本机未配置 Ghidra 12.1 / IDA MCP，Hopper 不支持 Windows，因此 REA 的函数/反编译类工具不可用；`vmix29.exe` 为 481 MB，也超过 REA 的 64 MB PE 资源与 128 MB 托管快照上限。vMix 的控件清单来自可读的 `languages/en.txt` 与 REA 对已安装 28 版程序集的托管元数据检查，而不是对 29 版安装包的反编译。

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

**本项目采用**：PGM 大、PVW 小的双监视器；`CUT` 与 `AUTO` 作为最大按钮；转场类型四个并列按钮（Fade / Wipe / Slide / Zoom）；`FTB` 独立开关；顶栏 `REC` / `STREAM` 胶囊指示；右侧输入缩略图列表（编号、名称、类型、PGM/PVW 标记）；`Add Input` 按采集类型给出参数。

## 对照表

| 界面元素 | 来源 | Mixvera 落点 |
| --- | --- | --- |
| 深色面板 + 蓝色强调 | OBS Yami | `src/mixvera/ui/theme.cpp` |
| Scenes / Sources 两栏 | OBS | `src/mixvera/ui/scene_panel.cpp` |
| 源显隐勾选 | OBS | 同上（`sourceList`） |
| 音频混音器 + 电平表 | OBS | `src/mixvera/ui/mixer_panel.cpp` |
| 录制 / 推流状态胶囊 | OBS + vMix | `main_window.cpp` 顶栏 `pillRec` / `pillStream` |
| PGM / PVW 双监视器 | vMix | `src/mixvera/ui/stage_view.cpp` |
| Cut / Fade / Wipe / Slide / Zoom / FTB | vMix | `src/mixvera/ui/transition_bar.cpp` |
| 输入缩略图列表 + 编号 | vMix | `src/mixvera/ui/input_panel.cpp` |
| Add Input 分类对话框 | vMix | `src/mixvera/ui/add_input_dialog.cpp` |

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
  'data\obs-studio\themes\Yami.obt' 'data\obs-studio\locale\en-US.ini'

# 配色 token（craft-anything core_text_read 亦可读取同一文件）
Select-String -Path build\analysis\obs\data\obs-studio\themes\Yami.obt -Pattern '^    --(blue3|red3|green2|grey[4-8])'

# OBS 界面语义
Select-String -Path build\analysis\obs\data\obs-studio\locale\en-US.ini -Pattern '^Basic\.Main\.'

# vMix 主窗口控件
Get-Content 'C:\Program Files (x86)\vMix\languages\en.txt' -Encoding UTF8 | Where-Object { $_ -like 'frmmain.*' }
```

`vmix29.exe` 是 Inno Setup 安装包，当前仓库没有内置解包器；不要为了取证直接运行安装器。需要 vMix 29 的控件变化时，先在隔离环境安装，再对安装目录中的 `languages/en.txt` 与 `vMix.exe` 做同样的只读检查。

vMix 程序集元数据通过 REA 的 `inspect_managed_artifact` 获得（证据 id `ev_72461980d1d0479e4c1419b2c29e5ecde420adc2933af8ef932735db1d2f6e9e`）。