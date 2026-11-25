# SinSam（芯象）逆向观察与 Mixvera 移植范围

这份文档记录 `sinsam_setup.26.7.10.3.exe` 的只读分析结果，以及 0.5.0–0.6.0 选择移植的功能。原则与 OBS / vMix 的研究相同：**只借鉴公开可观察的功能组织与交互语义，不复制代码、图标、字体、模板或文案**。安装包没有运行，所有提取物都在被 git 忽略的 `build/analysis/sinsam*` 目录中。

> **0.5.0 已实现**：4 路叠加层、Fly/Cube 转场、标题预置/倒计时/比分与快照，界面截图见 `docs/demo/mixvera-0.5-*.png`。
>
> **0.6.0 已实现**：竖屏/方形画布、显示模式、蓝绿幕抠像、字幕条/滚动字幕，以及 GT 标题包装（`.gtzip` / `.sinsamtitle` / `.xaml` 只读渲染、元素覆盖、命名预置、DataChangeIn），界面截图见 `docs/demo/mixvera-0.6-*.png`。实现细节见 [CHANGELOG.md](../CHANGELOG.md)。

## 取证对象

| 对象 | 识别结果 |
| --- | --- |
| `sinsam_setup.26.7.10.3.exe` | 32 位 NSIS 安装包，MSVC；SHA-256 `b822f9fcb4c3816b06c00bbe4d5bb97197be44de11837e25c2e39a46ddd3d6ee` |
| 安装包载荷 | `app.7z`、`titles.7z` / `vtitles.7z`（空）、`DefaultAppData.7z`、`.NET 4.8` 安装器、皮肤与辅助 DLL |
| `app/sinsam.exe` | x64 原生启动器；SHA-256 `1dfacd42de18aff22fb9203edb056e20ff10e57e899a740480d30bf66d4aaa16` |
| `app/sinsam.dll` | .NET 8.0 WPF 主程序集，版本 `26.7.10.3`，7,471 个类型、51,159 个方法、131 个程序集引用、608 个清单资源；SHA-256 `cbb255e2ef7fda3b48f5cbb35ae81536a1c47c91ee5366256432db2e116046d2` |
| `app/WebServer/SKILL.md` | 产品自称「SinSam 芯象专业级导播」，描述 119 个 MCP 工具与完整直播工作流；SHA-256 `3d1a7128642eb9f4fa7f4f7f5f8f7cee002f030462d4764cdbc39a6b102d1b4d` |
| `app/WebServer/Mcp/Generated/tools.json` | 221 个工具定义，12 个类别；SHA-256 `a71bb0fcc778c919379d134ab6c89eed82f6e237b67d7a1954f94893f11e31d7` |
| `app/titles/local/自定义文本.gtzip` | 标题模板 zip：`document.xml`、`resources.xml`、`thumbnail.png`、`[Content_Types].xml`；SHA-256 `0b6734cb766a15c56f7e7fb67c7d592610c5b5148e38ac05f9e84f2824fcfc68` |
| `C:\Program Files (x86)\vMix\titles` | vMix 28 本机 GT 标题库：743 个 `.gtzip` 与 87 个 `.xaml`，0.6 只读扫描 |

分析证据：`ev_9ce0db81…b6ab`（安装包 triage）、`ev_e131c0d1…f79d`（sinsam.exe）、`ev_93f51371…072a`（sinsam.dll 摘要）、`ev_e40a78be…398`（托管程序集，608 个资源）、`ev_0e12de4c…ea08` / `ev_199011be…5c13` / `ev_d9cb7b16…21be`（资源分页）、`ev_4e785dc5…0be0` / `ev_fe32cc72…f51c`（SKILL.md）、`ev_ce58510e…7ae1`（tools.json）、`ev_4be01685…5ec1`（gtzip）。

## 功能地图

608 个 WPF 资源名与 221 个 MCP 工具一起给出了完整的功能面：

| 模块 | 观察到的能力 | 证据 |
| --- | --- | --- |
| 输入 | 视频/音频文件、图片、幻灯片、网络流、VLC、屏幕捕捉、摄像头、DeckLink、AJA、音频设备、动态字幕、特效字幕、颜色层、浏览器、虚拟集、会议模板、播放列表、PPT/Word/Excel 课件 | `FrmSinSam`、`Select*Layout` 资源；SKILL.md `AddInput` 类型表 |
| 转场 | Cut、Fade、Zoom、Wipe、Fly、Cube、Merge、Stinger 1–4 | tools.json Transition 22 项 |
| 叠加 | 4 个叠加层、预览叠加、快捷叠加、停止叠加、画中画（MultiView，最多 9 图层引用） | tools.json Overlay 22 项；`FrmOverlay`、`SettingOverlayLayout` |
| 输出 | 推流、多路推流、4 路录制、外部输出、NDI/SRT/OMT/AJA/DeckLink 输出、截图、全屏/多屏投屏、录屏、放大镜、美颜 | tools.json Output 29 项；`FrmNDIOutputSetting`、`FrmSRTOutput`、`FrmSnapshoot` |
| 标题 | 预置切换、倒计时开始/暂停/停止、比分递增、文字/图片/颜色设置、动画时长、标题设计器与在线标题 | tools.json Title 5 项；`TitlePropertyCountDownLayout`、`TitleCountdownSettingForm` |
| 音频 | 通道开关、主/源监听、音量渐变、压缩/EQ/降噪/延迟、ASIO、矩阵与多路输出 | tools.json Audio 8 项；`FrmAudioConsoleSetting`、`AudioConsoleMatrixLayout` |
| PTZ | 八方向移动、变焦、自动/手动对焦、回位；Sony VISCA 模块 | tools.json PTZ 18 项；`PTZ/SinSam.Sony.Visca.dll` |
| 即时回放 | 84 个工具：多机位标记、片段选择、播放/导出、速度与方向 | tools.json Replay 84 项；`InstantReplay*` 资源 |
| 数据源 | Excel / JSON / RSS / Text / XML；上一行、下一行、选择行 | `datasources/*.dll`；tools.json DataSource 3 项 |
| 虚拟集 / 美颜 | 3D 虚拟集与机位、AI 抠像、美颜/美妆/滤镜、白板 | `SelectVirtualSet3D*`、`Beauty*`、`ImageMatting*`、`FrmWhiteBoard` |
| 会议 / 连麦 | 会议模板、P2P、连麦、腾讯会议、手机投屏 | `FrmTencentMeeting*`、`LinkMic*`、`P2P*`、`SelectPhone*Projection` |
| 控制 | 内置 MCP 服务器（119 个会话工具）、Web 控制页、快捷键、MIDI、激活器、Tally 灯 | `WebServer/`、`FrmRemoteControl`、`FrmActivator`、`TallyLight*` |
| 延时 | 延时输出、延时推流、事故回切 | tools.json DelayOutput 9 项 |

标题模板 `.gtzip` 是 zip + XML 的设计文件（`document.xml` + `resources.xml` + 缩略图）。0.5 使用内置预置；0.6 起以只读方式解析这套 GT 文档（见「0.6 追加：GT 标题包装」），仍不提供设计器，也不写回文件。

## 0.5.0 移植范围

选择标准：**切换台主路径上的高频能力、能在 Qt/C++ 中真实实现并验证、不依赖专有 SDK 或硬件**。

### 采用

1. **4 路叠加层（Overlay 1–4）** — 对应 sinsam 的 `OverlayInput1..4`、`PreviewOverlayInput1..4`。叠加层位于转场之后、FTB 之前；每层可指定任意输入（含场景），可独立开关；预览监视器可选择显示叠加结果。
2. **Fly 与 Cube 转场** — 对应 sinsam 的 `Fly`、`Cube`。Mixvera 原有 Cut / Fade / Wipe / Slide / Zoom，补齐到六种；Stinger 与 Merge 不移植（Merge 与 Fade 等价，Stinger 需要视频转场素材）。
3. **标题预置：下三分之一、全屏、倒计时、比分牌** — 对应 sinsam 的标题预置、倒计时与 `Increment`。提供内置样式与实时控制；0.6 另外加入 GT 包装读取与命名预置。
4. **截图（Snapshot）** — 对应 `Snaphost`：把当前节目输出（含叠加与 FTB）保存为 PNG。

### 暂不移植

| 能力 | 原因 |
| --- | --- |
| DeckLink / AJA / NDI / SRT / OMT 输入输出 | 需要厂商 SDK 与硬件，本仓库无对应依赖 |
| 多路推流、4 路录制、延时输出 | 需要重构输出管线；当前先保证单路录制/推流稳定 |
| PTZ（VISCA） | 无云台硬件可验证；先保留领域模型，后续单独实现 |
| 即时回放（84 个工具） | 需要环形缓冲与多机位同步录制，工程量独立 |
| 数据源、虚拟集、AI 抠像、美颜、会议、连麦、手机投屏 | 依赖外部模型、服务或协议，超出 0.5 范围 |
| 浏览器 / PPT / Word / Excel 输入 | Qt 6.8 MinGW 构建未包含 WebEngine，Office 渲染需要 COM |
| 内置 MCP 服务器、Web 控制页、MIDI、Tally、激活器 | 属于控制面而非切换台核心；现有 `control/` JSON-RPC 骨架保留 |
| 标题设计器（编辑并保存 `.gtzip` / `.pag`） | 0.6 已能只读渲染 GT 文档子集；设计器 UI 与 `.pag` / `.mov` 动画仍不在范围 |

## 0.6.0 取证：竖屏、抠像与字幕

0.6 的证据同样来自 `sinsam.dll`，但换了一层：中文功能名直接出现在 UTF-8 字符串块中，参数类型与设置方法来自托管元数据。下列偏移是 `sinsam.dll` 的文件内字节偏移，记录它们是为了复现；没有反编译方法体。

### 竖屏与画布

| 观察 | 证据（偏移） |
| --- | --- |
| 「竖屏功能」是按许可证区分的能力 | `竖屏功能` `0x874c3b`；同区的许可标签 `基础版` `0x874be4`、`免费试用版` `0x874bf3`、`高级版` `0x874c08`、`旗舰版` `0x874c17`、`最大分辨率` `0x874c26` |
| 横屏 / 竖屏是画布方向，不是窗口方向 | `横屏` `0x876d79`、`竖屏` `0x876d85`；导入提示 `1080*1920` `0x8769df` |
| 另有工作区模式切换 | `切换界面模式` `0x875e3b`、`切换界面布局` `0x875df3`、`专业模式` `0x876d55`、`电商模式` `0x876d67` |

**采用**：把「横屏 / 竖屏」理解为画布方向，提供 1080×1920、720×1280、1080×1080 预设、视图菜单一键交换长短边、`--canvas` 参数；画布尺寸贯通合成、标题、录制、推流与快照。

**不采用**：`切换界面模式 / 切换界面布局` 与 `专业模式 / 电商模式` 属于工作区布局与许可分层，不是切换台核心；Mixvera 保持横向工作区，只让画布方向可变。

### 抠像

| 观察 | 证据 |
| --- | --- |
| 功能入口按抠像类型分组 | 字符串 `蓝绿专用抠图` `0x87537e`、`任意颜色抠图` `0x875396`、`高级抠图` `0x8753ae` / `0x8775d7`、`区域抠图改变` `0x875c1f`、`抠图初始化` `0x875c37` |
| 参数对象与「key cleaner」 | 托管名 `VZ_ChromaKeyParam` `0x78a72f`、`VZ_KeyCleanerParam` `0x78a741`、`ChromaKeyMatting` `0x78d85c`、`ChromaKeyMattingColorType` `0x78d88e` |
| 设置参数与遮罩的 API | `SetChromaKeyMattingParam` `0x7ee2d8`、`SetKeyCleanerParam` `0x7ec367`、`SetChromaKeyMask` `0x7ee3bb` |
| 区域遮罩是独立参数与窗体 | `VZ_ChromaKeyAreaParam` `0x78a719` / `0x7f9ee8`、`FrmChromaKeyMaskPoint` `0x7900ed` / `0x845d74`；遮罩形状字符串 `线性` / `镜面` / `圆形` / `矩形` / `爱心` / `星形` |

**采用**：键色（绿 / 蓝 / 自定义）+ 容差（相似度）+ 边缘（平滑度）+ 溢色抑制（对应 `KeyCleaner` 的语义）。算法在归一化 RGB 色度上比较，整幅实时处理，参数随工程保存，调参不重建采集源。

**不采用**：区域抠图、异形遮罩与 `SetChromaKeyMask` 的遮罩控制；这需要遮罩编辑 UI 与逐帧蒙版，留作后续。AI 抠像 / 美颜依赖外部模型，同样不在 0.6。

### 字幕

| 观察 | 证据 |
| --- | --- |
| 字幕分本地与在线 | `本地字幕` `0x874cc2`、`在线字幕` `0x874cd4` |
| 字幕有独立编辑入口与文案概念 | `字幕编辑` `0x87180b`、`字幕文案` `0x87181d`、`字幕设计编辑器` `0x874e67` |
| 编辑态与预设选择 | `字幕实时更新按钮状态改变`、`选择字幕选择预设文本`；MCP 标题工具把标题称作「字幕」，并提供 `SetText`、`SetColour`、`SetCountdown`、`GetTitleElementProps` |
| 滚动字幕与语音字幕是两种标题类型 | `TitleTicker` `0x78d7b0`、`TitleSpeech` `0x78d7a4`、`AISpeechSource` `0x79bbc2` |
| 标题格式 | 标题模板 `.gtzip` / `.sinsamtitle` / `.xaml`（0.6 只读渲染），特效字幕 `.pag`（不支持） |

**采用**：本地字幕的「字幕文案 + 预设切换」语义——标题新增字幕条与滚动字幕两种样式，多行预置文案，输入条右键设置 / 下一句 / 上一句 / 清除；滚动速度可调；排版按画布宽度自适应。MCP 工具里的 `SetText` / `SetColour` 对应 Mixvera 的标题文字与强调色参数。

**不采用**：在线字幕、`TitleSpeech` 与 `AISpeechSource`（需要在线语音服务）；字幕设计器与 `.pag` 特效字幕 / `.mov` 透明动画（需要专用引擎）；MCP 控制面本身。`.gtzip` / `.sinsamtitle` / `.xaml` 的只读渲染在 0.6 追加。

### 取证限制

- 以上结论来自字符串与托管元数据，**属于观察 + 推断**；没有运行 SinSam，也没有逐个反编译方法体，因此无法确认参数默认值、算法细节或界面交互的完整行为。
- 偏移只对 SHA-256 `cbb255e2…046d2` 的这份 `sinsam.dll` 有效；换版本必须重新定位。
- 0.6 的抠像参数默认值（35 / 18 / 60）是 Mixvera 自己的调参结果，不是从 SinSam 读出的默认值。

## 0.6 追加：GT 标题包装

0.6 的核心证据不是 SinSam 的私有代码，而是 **vMix 28 安装目录里的 GT 模板库**：`titles/` 下 743 个 `.gtzip`（GT Title 400、GT Vertical 147、GT Stingers 66、GT Social 63、GT ScoreBoard 18、GT Text 18、GT Timer 17、GT Ticker 14）与 87 个 `.xaml`。全部只读解包扫描，没有运行 vMix / GT Designer，也没有把模板纳入 git。SinSam 的字幕导入过滤器（`.xaml;.gtzip`，描述中另有 `.sinsamtitle`，`0x87678a`；100 MB 上限 `0x8767d1`）确认了它读的是同一套 GT 文档。

### 语料结论

| 观察 | 数据 | Mixvera 的解析 |
| --- | --- | --- |
| 包结构 | zip 内 `document.xml`、`resources.xml`、`thumbnail.png`、`[Content_Types].xml` | 读前两者；图片相对文档目录解析 |
| 元素 | `Rectangle` 2378、`Composition` 2234、`Layer` 1491、`TextBlock` 1279、`Ellipse` 415、`Image` 387、`Ticker`、`QRCode`、三角形 | 全部实现；未知元素跳过 |
| 画笔 | `LinearGradient` 605、`Transparent` 77、`Bitmap` 73、`RadialGradient` 3；`Brush.Color` 出现 8654 次，`Type` 只有 758 次 | 缺 `Type` 时按子节点 / 属性推断；缺 `Color` 视为透明 |
| 变换 | `Transform Rotate="x,y,z"`，值集中在 0–6.28 | 取 z 分量按弧度；>6.5 视为角度 |
| 动画 | `Fly` 2547、`Fade` 2317、`None` 1040、`Reveal` 612、`Expand` 568；`Fly.Duration` 缺失 95 次 | 缺省 `Duration` 0.5 s；`None` 不产生动画 |
| Storyboard | `Type` 空 1020 次，另有 `TransitionOut`、`DataChangeIn/Out` | 四类分别用于入场、出场、预置切换 |
| Ticker | `Direction` 多为空，少量 `Top` | 空 = 水平；top/bottom/up/down/vertical = 旋转 −90° 竖向 |
| 图片 | `Stretch="Fill"`、`SizeMode="Stretch"` | 映射为铺满 |
| 时钟 | `{0:HH:mm}`、`{0:hh:mm tt}`、`{0:hh:mm:ss tt}`、`{0:4:00PM\|mm:ss}`、`{0:h:mm:ss}` | .NET 自定义格式子集 + `\|` 倒计时段 |
| 竖屏艺术板 | GT Vertical 的 `3412×1920`，中心 `1080×1920` 画面 | 按高度缩放并居中裁切 |

### 支持矩阵

| GT 能力 | 0.6 | 说明 |
| --- | --- | --- |
| `.gtzip` / 未压缩目录 / `.xaml` / `.sinsamtitle` | 支持 | 只读；不写回 |
| Composition / Layer / Rectangle / Ellipse / Triangle / RightTriangle / TextBlock / Image / Ticker / QRCode | 支持 | 含圆角、描边、透明度、旋转 |
| Solid / LinearGradient / RadialGradient / Transparent / Bitmap 画笔 | 支持 | 渐变起点 / 终点按文档坐标 |
| Fade / Fly / Reveal / Expand / ZoomFade / Zoom / Bounce / Rotate | 支持 | 含 Delay / Reverse / cubic 缓动 |
| 入口 / TransitionOut / DataChangeIn / DataChangeOut | 支持 | 预置切换播放 DataChangeIn |
| Ticker 水平 / 竖向 | 支持 | 取模板中最宽的 TextBlock |
| `{0:...}` 时钟 / 倒计时 | 支持 | .NET 格式子集 |
| QRCode | 支持 | 真实 QR（ECC M，版本 1–10） |
| GT `Effect`（模糊 / 阴影 / 描边发光） | 不支持 | 不渲染 |
| `DataFlags` 数据绑定 | 不支持 | 不绑定外部数据源 |
| `.pag` / `.vzae` 特效字幕、`.mov` 透明动画 | 不支持 | 需要专用解码与动画引擎 |
| 在线标题、AI 语音字幕 | 不支持 | 依赖在线服务 |
| GT 设计器（编辑并保存 `.gtzip`） | 不支持 | 只覆盖文本 / 颜色 / 图片元素 |

### 界面取舍

- 包装选择放在「添加输入 → 标题 → 标题包装」：路径输入 + `Browse…` + `Elements…`，与 vMix「Title 输入 + 元素列表」的信息结构一致，但不做设计器画布。
- 「Elements…」对应 SinSam 的 `GetTitleElementProps` 与 `SetText` / `SetImage` / `SetColour`：只列文本 / 图片 / 颜色元素，逐项覆盖，写进工程参数而不是包装文件。
- 命名预置对应 `NextTitlePreset` 与 `选择字幕选择预设文本`（`0x87572c`）：最多 10 组，组名可编辑；切换时播放包装自己的 `DataChangeIn`，与 vMix 的标题预置切换行为一致。

### 取证限制

- GT 语料来自本机已安装的 vMix 28，只解析 XML 结构与属性分布；没有运行 vMix 或 GT Designer，也没有逐模板比对渲染结果。元素 / 画笔 / 动画的语义有语料支撑，像素级排版仍是按 GT 文档推断实现。
- GT Vertical 的 `3412×1920` 画板与中心 1080×1920 区域来自对模板 `Composition` 的统计；个别模板若采用别的画板比例，按普通 `contain` 处理。
- SinSam 的 `.sinsamtitle` 只在导入过滤器描述中出现，实际 pattern 是 `*.xaml;*.gtzip`；Mixvera 按散装 GT XML 读取，未验证 SinSam 自身的兼容行为。

## 与 Mixvera 现有能力的对应

| Mixvera 已有 | sinsam 对应 | 0.5–0.6 变化 |
| --- | --- | --- |
| 场景 / 图层、PVW/PGM、Cut/Auto/T-Bar、FTB | 场景切换、转场、T-Bar | 追加 Fly / Cube |
| 输入条、场景栏、调音台 | 数据源列表、机位、音频 | 增加叠加层分配入口 |
| 标题输入（纯文本） | 标题预置 / 倒计时 / 比分 / 字幕包装 | 0.5 四种样式；0.6 字幕条、滚动字幕、GT 包装与命名预置 |
| 画布（横屏） | 横屏 / 竖屏、最大分辨率 | 0.6 增加竖屏 / 方形预设、一键切换与 `--canvas` |
| 输入（无抠像） | 蓝绿专用 / 任意颜色 / 高级抠图 | 0.6 增加整幅色度键控与溢色抑制 |
| 录制 / 推流 | 录制 / 推流 | 增加截图；输出尺寸跟随画布 |
| `control/` JSON-RPC `GetStatus` | 内置 MCP 服务器 | 暂不扩展 |

## 复现取证

```powershell
# 1. 只读解包 NSIS 安装包（Bandizip 支持 NSIS），输出到被 git 忽略的目录
& 'C:\Program Files\Bandizip\bz.exe' x -y -o:build\analysis\sinsam `
  'D:\Mixvera\sinsam_setup.26.7.10.3.exe'

# 2. 解出主程序包与标题模板
& 'C:\Program Files\Bandizip\bz.exe' x -y -o:build\analysis\sinsam-app `
  'build\analysis\sinsam\app.7z'

# 3. 查看 MCP 工具目录（221 个工具）
python -c "import json; d=json.load(open(r'build\analysis\sinsam-app\app\WebServer\Mcp\Generated\tools.json',encoding='utf-8')); print(len(d))"

# 4. 查看标题模板结构（只读）
& 'C:\Program Files\Bandizip\bz.exe' l `
  'build\analysis\sinsam-app\app\titles\local\自定义文本.gtzip'

# 5. 读取 0.6 证据字符串（UTF-8，只读字节；不执行 DLL）
python -I -c "d=open(r'build\analysis\sinsam-app\app\sinsam.dll','rb').read(); [print(hex(o), d[o:d.find(b'\0',o)].decode('utf-8','replace')) for o in (0x874c3b,0x87537e,0x874cc2,0x78d7b0)]"

# 6. 只读扫描 vMix 28 的 GT 模板库（不运行 vMix；结果不入库）
python -I -c "import glob,zipfile; fs=glob.glob(r'C:\Program Files (x86)\vMix\titles\**\*.gtzip',recursive=True); print(len(fs), sum('document.xml' in zipfile.ZipFile(f).namelist() for f in fs)); print(len(glob.glob(r'C:\Program Files (x86)\vMix\titles\**\*.xaml',recursive=True)))"
```

`app/sinsam.exe`、`sinsam.dll` 与所有解包文件都不会进入 git；`.gitignore` 的 `/*.exe` 规则同时覆盖根目录的安装包。

## 限制

- 没有运行安装器或任何解包程序，因此没有运行中的界面截图；界面结构来自 WPF 资源名、MCP 工具目录与官方 `SKILL.md`。
- 主程序集 12.8 MB、7,471 个类型，未逐个反编译方法体；功能结论来自资源名、工具定义与文档，属于**观察 + 推断**，不声称覆盖全部内部行为。
- `sinsam.dll` 的托管成员检查因 64 MB 表示预算被 REA 拒绝；资源清单仍由 `inspect_managed_artifact` 完整返回。
- 版本号、产品名来自程序集元数据与 `SKILL.md`；未验证在线服务、授权或硬件激活器行为。
