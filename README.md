# Mixvera

Mixvera 是一个面向现场导播场景的开源切换台。`0.4.0` 在 0.3 的中文界面与 vMix 式工作区之上，完成**主题、调音台与设置界面**的设计：三套可实时预览的主题、OBS 式通道条调音台、按 OBS 分类组织的设置对话框。

![Mixvera 0.4 中文炭灰主题](docs/demo/mixvera-0.4-zh-charcoal.png)

Yami（OBS 蓝灰）与 Daylight（浅色）主题：

![Mixvera 0.4 Yami 主题](docs/demo/mixvera-0.4-en-yami.png)

![Mixvera 0.4 Daylight 主题](docs/demo/mixvera-0.4-zh-daylight.png)

## 0.4 新增

### 三套主题

- **Charcoal**（默认）：vMix 式炭灰表面 + 橙色控制强调，预览橙 / 节目绿保持信号色
- **Yami**：取自 OBS `Yami.obt` 的蓝灰三级表面（`#13141A` / `#1D1F26` / `#272A33`）与蓝色强调 `#284CB8`
- **Daylight**：浅色高对比界面，适合明亮导播间；监视器仍为近黑底，画面不受主题影响
- 47 个颜色/图标 token 驱动一份 QSS 模板与全部自绘控件，界面里没有散落的颜色字面量
- 设置里选中主题**即时预览**，取消会恢复原主题；`--theme charcoal|yami|daylight` 或 `MIXVERA_THEME` 可指定

### 调音台

- OBS 式通道条：编号、名称、增益 dB、竖向推子（-60…+12 dB）、`M` / `S`
- 双电平平段的主输出表（绿/黄/红）带刻度、峰值保持与 dB 读数
- 推子双击复位 0 dB；静音与增益随工程保存，独奏为监听状态
- 电平由系统默认录音设备真实驱动；没有设备时显示 `NO AUDIO INPUT`

### 设置界面

- 左侧分类导航 + 右侧分页：常规、视频、音频、输出、快捷键、关于
- 主题卡片带实时缩略图；「跟随系统语言」与「启动时恢复上次会话」是真实生效的设置
- 视频页设置画布预设/分辨率/帧率，输出页管理录制目录、ffmpeg 与 RTMP
- 统一样式的主/次按钮、下拉与微调箭头图标，三套主题下都清晰

设置界面截图：[常规（中文）](docs/demo/mixvera-0.4-settings-general-zh.png)、[音频（英文）](docs/demo/mixvera-0.4-settings-audio-en.png)、[视频（中文）](docs/demo/mixvera-0.4-settings-video-zh.png)。

## 0.3 新增

### 简体中文

- 全部界面文案（213 条）已翻译为简体中文，启动时自动按系统语言选择；中文系统直接显示中文
- `设置 → 界面语言` 可在「简体中文 / English」之间切换，下次启动生效
- 也可用命令行 `--lang zh_CN`、环境变量 `MIXVERA_LANG=zh_CN` 强制指定
- 翻译以 Qt `tr()` + `.ts` / `.qm` 管理，`.qm` 通过 Qt 资源内嵌进 exe，分发时无需额外语言文件

英文界面截图见 [docs/demo/mixvera-0.3-en.png](docs/demo/mixvera-0.3-en.png)。

术语优先采用 OBS 中文语言包的专业说法（Cut → 硬切、Fade → 淡入淡出、Wipe → 划像、Stream → 推流），没有沿用 vMix 机器翻译里的「切 / 褪色 / 擦拭 / 流」。

### vMix 式工作区

- 预览在左（橙色标题条）、节目在右（绿色标题条），中央是竖向转场区：`硬切` / `自动` / 淡入淡出 / 划像 / 滑动 / 缩放 / FTB / T-BAR
- 底部横向输入条：缩略图、编号、名称、PGM/PVW 状态与快捷操作；分类筛选与搜索
- 底部功能栏：`+ 添加输入`、`录制`、`推流`；顶栏可显示/隐藏场景与调音台
- 配色直接取自 vMix 主窗口：背景 `#1E2328`、转场面板 `#404B56`、预览橙 `#FF8C00`、节目绿 `#006400`

## 这一版能做什么

### 真实输入

| 输入 | 说明 |
| --- | --- |
| 屏幕采集 | 按显示器序号抓取整个桌面 |
| 窗口采集 | 枚举可见顶层窗口，优先抓窗口本体，失败回退到屏幕区域 |
| 摄像头 | 通过 Qt Multimedia 采集，系统里没有摄像头时列表为空 |
| 图片 / 视频文件 | 图片直接显示；视频循环播放，画面进入合成 |
| 标题 | 支持多行文本、字号、对齐、颜色 |
| 纯色 / 测试图案 | 用于垫场与信号检查，测试图案带活动扫描光带和时码 |

### 场景与图层

- OBS 式「场景 / 源」两栏：场景列表、当前场景的图层列表
- 图层支持显隐开关、上移 / 下移、从场景移除
- 场景可新增、复制、重命名、删除
- 工程以 JSON 保存，退出自动保存、启动自动恢复（`%APPDATA%/Mixvera/project.json`）

### 导播与转场

- PVW / PGM 双监视器左右并排，画面来自同一条渲染时间线
- `硬切`、`自动`、`T-BAR` 手动推子（过半完成、不过半回退）
- 四种转场：淡入淡出 / 划像 / 滑动 / 缩放
- `FTB` 淡出到黑
- 转场时长 100–5000 ms 可调

### vMix 式输入条

- 横向滚动的输入条：实时缩略图、输入编号、名称与类型
- 每块输入带 `PVW` / `PGM` / 加入场景 / 属性 / 移除快捷按钮
- `PGM` 绿色、`PVW` 橙色标题条；双击打开属性；右键菜单提供同样的操作
- 按「全部 / 场景 / 采集 / 媒体」筛选，可搜索输入名称
- 「添加输入」对话框按类型给出对应参数（显示器、窗口、摄像头、文件、标题文本、颜色）

### 音频

- 右侧可收起的调音台：每个带音频的输入一条通道，推子（-60…+12 dB）、静音、独奏
- 增益与静音随工程保存；独奏只影响监听状态
- 主输出电平表由系统默认录音设备实时驱动（不是动画），带刻度与峰值保持
- 没有输入设备时明确显示「无音频输入」

### 输出

- `RECORD`：把合成后的 PGM 画面通过 ffmpeg 管道编码为 H.264 MP4
- `STREAM`：同一路画面推 RTMP（URL 与串流密钥在设置里配置）
- 顶栏 `REC` / `STREAM` 胶囊显示计时，状态栏显示 ON AIR / STANDBY
- 输出目录默认 `%USERPROFILE%/Videos/Mixvera`，可在设置里改成绝对路径

## 快捷键

| 按键 | 动作 |
| --- | --- |
| `Space` | 硬切 |
| `Enter` | 自动转场 |
| `F11` | 全屏 / 退出全屏 |
| `Esc` | 退出全屏 |
| `Ctrl+R` | 开始 / 停止录制 |
| `Ctrl+I` | 添加输入 |
| `Ctrl+S` / `Ctrl+O` | 保存 / 打开工程 |

## 界面来源

界面不是凭印象画的，颜色、控件命名与信息组织取自 OBS Studio 33.0.3 安装包（本机安装为 32.1.2）与 vMix 28 的可读产物。0.4 的 Yami 主题、调音台通道条与设置分类直接对应这些证据：

- **vMix 主窗口**：官方帮助页的主界面截图（`build/analysis/vmix/NewItem446.png`，本机分析目录，未入库）用于确定预览左 / 节目右的布局、中央竖向转场列、底部横向输入条与功能栏；从截图采样得到背景 `#1E2328`、转场面板 `#404B56`、预览橙 `#FF8C00`、节目绿 `#006400`、页脚 `#293038`、状态栏 `#0A0F14`
- **vMix 语言包**：`languages/en.txt` 的 `frmmain.*` 控件清单（Cut / Fade / Wipe / Slide / Zoom / FTB、Add Input、Overlay、Record、Stream、Audio Mixer、Settings）与 `zh.txt` 的中文条目，用于确认功能集合和中文术语取舍
- **OBS**：`Yami.obt` 的三级深色表面与 `--blue3` 强调色成为 0.4 的 Yami 主题；`volMeterFrame`、`vMixerScrollArea` 等对象名与混音器语义对应通道条设计；`locale/zh-CN.ini` 提供专业中文术语（硬切、淡入淡出、划像、推流、录制等）
- **设置分类**：OBS「常规 / 视频 / 音频 / 输出 / 快捷键 / 关于」的分页方式成为 0.4 设置对话框的信息架构

完整的对照与取舍见 [docs/UI-SPEC.md](docs/UI-SPEC.md)，主题 token、布局尺寸、调音台与设置的行为约定见 [docs/DESIGN.md](docs/DESIGN.md)。两份程序的原工程结构复原（OBS 的 CMake / Qt / libobs / 插件树，vMix 的 VB.NET 模块与窗体清单）见 [docs/RE-PROJECTS.md](docs/RE-PROJECTS.md)。没有复制任何 OBS 或 vMix 的代码、图标或资源；只借鉴公开可观察的界面组织与配色值。

> vMix 28 已安装但未购买授权，直接启动会停在 “License Request” 对话框，因此无法对运行中的界面截图；布局证据来自官方帮助截图。vMix 29 安装包只做了只读识别，没有运行安装器。

## 环境要求

- Windows 10/11 x64
- Qt 6.8.3 MinGW（Core / Gui / Widgets / Multimedia）
- MinGW 13.1、CMake ≥ 3.28、Ninja
- ffmpeg（录制与推流需要，需在 `PATH` 中）

## 构建与运行

```powershell
cmake --preset windows-mingw
cmake --build --preset windows-mingw
ctest --test-dir build/mingw --output-on-failure
```

直接运行（Qt 已在 PATH 时）：

```powershell
.\build\mingw\Mixvera.exe
```

中文系统会自动显示中文；也可以显式指定语言与主题：

```powershell
.\build\mingw\Mixvera.exe --lang zh_CN
.\build\mingw\Mixvera.exe --lang en --theme yami
.\build\mingw\Mixvera.exe --theme daylight --show-mixer
```

界面截图可完全用命令行复现（参数表见 [docs/DESIGN.md](docs/DESIGN.md#截图与验收命令行)）：

```powershell
.\build\mingw\Mixvera.exe --no-restore --lang zh_CN --theme charcoal --show-mixer --screenshot docs\demo\shot.png --screenshot-delay 2800
.\build\mingw\Mixvera.exe --no-restore --lang zh_CN --settings-screenshot docs\demo\settings.png --settings-page 0
```

打包成可脱离 Qt 环境运行：

```powershell
D:\Qt\6.8.3\mingw_64\bin\windeployqt.exe --release --no-translations --compiler-runtime build\mingw\Mixvera.exe
```

`windows-mingw` 预设里的 Qt、MinGW、Ninja 路径在 `CMakePresets.json`，可按本机安装位置修改。

### 更新翻译

```powershell
# 1. 提取新字符串（会保留已有译文）
D:\Qt\6.8.3\mingw_64\bin\lupdate.exe apps src -ts translations\mixvera_zh_CN.ts

# 2. 检查 translations\mixvera_zh_CN.ts：所有 context 必须是
#    mixvera::ui::类名（Qt 运行时按命名空间查找译文）
# 3. 编译并重新构建
D:\Qt\6.8.3\mingw_64\bin\lrelease.exe translations\mixvera_zh_CN.ts -qm translations\mixvera_zh_CN.qm
cmake --build build\mingw
```

## 工程文件

工程是纯 JSON（`core/project.cpp` 定义 schema）：画布尺寸与帧率、全部输入及其采集参数、场景图层几何、PGM/PVW、转场设置、输出配置。`File → Open Project…` 可打开任意工程，`--project` 参数可在启动时载入。

## 测试

四个测试目标，全部通过：

| 目标 | 覆盖 |
| --- | --- |
| `mixvera_core_tests` | 切换与 T-Bar 释放逻辑 |
| `mixvera_project_tests` | 工程 JSON 往返、默认工程合法性、图层几何 |
| `mixvera_ui_smoke_tests` | 真实界面：默认工程、CUT/AUTO/T-Bar、监视器出帧、场景选择不递归、界面录制落盘、三套主题样式表完整且应用到主窗口 |
| `mixvera_media_tests` | 屏幕采集出帧、窗口枚举、采集画面合成非黑、ffmpeg 录制文件可生成 |

## 当前边界

- 录制为**无声视频**：音频链路目前只做电平监测、增益与静音状态，尚未混入输出
- 未接入 SDI / DeckLink / NDI 采集；相应输入类型在领域模型里预留但未实现
- 场景图层目前按「整幅」放置，裁剪、缩放、拖拽定位的编辑器还没做
- RTMP 推流需要外部地址；推流与录制共用一条 ffmpeg 原始帧管道
- 依赖外部 `ffmpeg.exe`，仓库不内嵌二进制

## 目录

```text
apps/mixvera          应用入口
src/mixvera/core      领域模型、工程序列化、事件总线、服务容器
src/mixvera/services  输入、切换、转场、音频、输出、回放、设备
src/mixvera/engine    媒体引擎抽象（Stub，可选 libobs）
src/mixvera/ui        Qt 界面、媒体采集与合成、录制
translations          Qt .ts 源文件、编译后的 .qm 与资源清单
resources             内嵌图标等 Qt 资源
tests                 core / project / ui / media 测试
```

详见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)。

## libobs 模式

准备好 libobs 的 CMake package 后：

```powershell
cmake -S . -B build-libobs -G Ninja -DCMAKE_PREFIX_PATH="D:/Qt/6.8.3/mingw_64;C:/path/to/obs/build" -DMIXVERA_ENABLE_LIBOBS=ON
cmake --build build-libobs
```

当前仅建立构建开关与引擎抽象，`LibobsMediaEngine` 适配层尚未实现。

## 许可

第三方依赖与许可说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。若将来链接 libobs，分发需遵守 GPL-2.0-or-later。