# SinSam（芯象）逆向观察与 Mixvera 移植范围

这份文档记录 `sinsam_setup.26.7.10.3.exe` 的只读分析结果，以及 0.5.0 选择移植的功能。原则与 OBS / vMix 的研究相同：**只借鉴公开可观察的功能组织与交互语义，不复制代码、图标、字体、模板或文案**。安装包没有运行，所有提取物都在被 git 忽略的 `build/analysis/sinsam*` 目录中。

> **0.5.0 已实现**：本文档「0.5.0 移植范围」中列出的四项功能（4 路叠加层、Fly/Cube 转场、标题预置/倒计时/比分、快照）已全部落地并通过测试，界面截图见 `docs/demo/mixvera-0.5-*.png`。实现细节见 [CHANGELOG.md](../CHANGELOG.md)。

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

标题模板 `.gtzip` 是 zip + XML 的设计文件（`document.xml` + `resources.xml` + 缩略图），属于设计器私有格式，本项目不读取也不兼容。

## 0.5.0 移植范围

选择标准：**切换台主路径上的高频能力、能在 Qt/C++ 中真实实现并验证、不依赖专有 SDK 或硬件**。

### 采用

1. **4 路叠加层（Overlay 1–4）** — 对应 sinsam 的 `OverlayInput1..4`、`PreviewOverlayInput1..4`。叠加层位于转场之后、FTB 之前；每层可指定任意输入（含场景），可独立开关；预览监视器可选择显示叠加结果。
2. **Fly 与 Cube 转场** — 对应 sinsam 的 `Fly`、`Cube`。Mixvera 原有 Cut / Fade / Wipe / Slide / Zoom，补齐到六种；Stinger 与 Merge 不移植（Merge 与 Fade 等价，Stinger 需要视频转场素材）。
3. **标题预置：下三分之一、全屏、倒计时、比分牌** — 对应 sinsam 的标题预置、倒计时与 `Increment`。提供内置样式与实时控制，不兼容 `.gtzip` 设计器格式。
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
| `.gtzip` 标题设计器格式 | 私有二进制/XML 格式，且需要设计器 UI；改用内置预置 |

## 与 Mixvera 现有能力的对应

| Mixvera 已有 | sinsam 对应 | 0.5 变化 |
| --- | --- | --- |
| 场景 / 图层、PVW/PGM、Cut/Auto/T-Bar、FTB | 场景切换、转场、T-Bar | 追加 Fly / Cube |
| 输入条、场景栏、调音台 | 数据源列表、机位、音频 | 增加叠加层分配入口 |
| 标题输入（纯文本） | 标题预置 / 倒计时 / 比分 | 扩展为四种样式与实时控制 |
| 录制 / 推流 | 录制 / 推流 | 增加截图 |
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
```

`app/sinsam.exe`、`sinsam.dll` 与所有解包文件都不会进入 git；`.gitignore` 的 `/*.exe` 规则同时覆盖根目录的安装包。

## 限制

- 没有运行安装器或任何解包程序，因此没有运行中的界面截图；界面结构来自 WPF 资源名、MCP 工具目录与官方 `SKILL.md`。
- 主程序集 12.8 MB、7,471 个类型，未逐个反编译方法体；功能结论来自资源名、工具定义与文档，属于**观察 + 推断**，不声称覆盖全部内部行为。
- `sinsam.dll` 的托管成员检查因 64 MB 表示预算被 REA 拒绝；资源清单仍由 `inspect_managed_artifact` 完整返回。
- 版本号、产品名来自程序集元数据与 `SKILL.md`；未验证在线服务、授权或硬件激活器行为。
