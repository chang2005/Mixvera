# OBS Studio 33.0.3 与 vMix 29 原工程复原报告

这份文档回答一个问题：**只看发行产物，能把这两人程序原来的工程结构复原到什么程度？**

结论先说：

- **OBS Studio 33.0.3** 是开源的 C/C++ 工程，构建系统是 CMake + Qt 6，代码分为 `libobs` 核心、`frontend` 前端、`shared` 共享控件、`deps` 内置依赖和 27 个核心插件。发行包里带了完整 PDB，因此原工程的目录树、构建目录、依赖版本和大部分源文件名都能直接读出来。
- **vMix 29** 是闭源的 VB.NET WinForms 程序，目标框架 .NET Framework 4.0，主程序集 `vMix.exe` 29.0.0.50 内含 5,218 个类型、31,958 个方法和 152 个托管资源。原解决方案的边界可以从程序集引用还原为 20 多个内部模块（采集、图形、编码、NDI、API 等），但 `.sln` / `.vbproj`、源码文件名和窗体精确坐标都无法从二进制恢复，且代码经过 .NET Reactor 混淆。

全程只读：没有运行两个安装器，也没有加载或执行任何目标程序集。

---

## 1. 取证对象与方法

### 1.1 对象

| 对象 | 路径 | 大小 | SHA-256 |
| --- | --- | ---: | --- |
| OBS 33.0.3 安装包 | `OBS-Studio-33.0.3-Windows-x64-Installer.exe` | 193,809,816 B | `700be33c01468c2a1e43820bbcc49ba914f570f0f82250022bb680fba43b2a67` |
| OBS 主程序 | `build/analysis/obs-app/bin/64bit/obs64.exe` | 5,609,232 B | `800924d5a9300ff6f0de227cd870bbac86790af5e93899d6740928e2013a359b` |
| OBS 核心库 | `build/analysis/obs-app/bin/64bit/obs.dll` | 1,191,184 B | `4aa1efd403d8c61a54c2e72d04c9b94dad9d53f0d96cd5d6e61b2fc218b67f0f` |
| OBS 前端 API | `build/analysis/obs-app/bin/64bit/obs-frontend-api.dll` | 41,744 B | `fcbb55ec241f77a2b1edccaae8992be76faaf44933cb478e39835c0ba84aa896` |
| vMix 29 安装包 | `vmix29.exe` | 481,764,296 B | `48624286f53fcbbcfd1ce7a0c81f8bf8ac2bb2be6a6e68ffcc1d9da86a00e467` |
| vMix 29 主程序 | `build/analysis/vmix29/app/vMix.exe` | 31,471,096 B | `3a88abd5079312222afa7916488b271a3d4ad6b204200ed3bb56f1b177259acd` |
| vMix 28 主程序（对照） | `C:\Program Files (x86)\vMix\vMix.exe` | 33,248,256 B | `7b9168ae689f810e7257cd665ef1d473f3827a08f98c08cbb71b33c602bf31d8` |

OBS 安装包是 NSIS 3 Unicode，用 7-Zip 26.01 只读列出 2,304 个文件；vMix 29 安装包是 Inno Setup 6.1.0 Unicode，用 `innoextract 1.9` 只读解出 `app/vMix.exe` 与 `app/languages/zh.txt`。两个安装器都没有执行。

### 1.2 工具分工

| 证据类型 | 工具 | 说明 |
| --- | --- | --- |
| OBS PE 资源树、图标组 | REA `inspect_pe_resources` | 16 个资源、36 个目录项，覆盖完整 |
| OBS 原生文件识别 | craft-anything `core_target_triage` / `core_overview_show` | 9,514 个函数、26,102 个字符串、31 个导入库 |
| OBS 导入/导出/版本/清单 | `pefile 2024.8.26` 只读脚本 | 报告生成在 `build/analysis/pe-report-obs.txt` |
| OBS 原工程路径 | PDB ASCII 扫描（`build/analysis/scan_pdb.py`） | `obs64.pdb` 46.7 MB、`obs.pdb` 5.8 MB，另加 5 个插件 PDB |
| vMix 托管程序集 | REA `inspect_managed_artifact` | 版本、CLR 头、元数据行数、资源、程序集引用 |
| vMix 主窗体成员 | REA `inspect_managed_members` | `vMix.frmmain` 的字段/方法/用户字符串 |
| vMix 29 资源清单 | REA `inspect_analysis_view` | 152 条 ManifestResource 全量分页 |

**工具限制**：本机 REA 的原生分析 provider（Ghidra）没有配置，`binary_overview` / `list_segments` / 反编译在本次不可用。因此 OBS 的原生代码结论来自 PE 结构、PDB、导入导出表和字符串，而不是反编译。这个限制不影响“原工程结构”的复原，但意味着没有函数级伪代码。

---

## 2. OBS Studio 33.0.3

### 2.1 二进制身份

`obs64.exe` 的版本资源直接给出产品身份：

```text
CompanyName     = OBS
FileDescription = OBS Studio
FileVersion     = 33.0.3
ProductVersion  = 33.0.3
InternalName    = obs
OriginalFilename= obs
```

PE 结构为 PE32+ / x86-64 / GUI 子系统，8 个节：`.text`、`.rdata`、`.data`、`.pdata`、`.detourc`、`.detourd`、`.rsrc`、`.reloc`。`.detourc` / `.detourd` 是 Microsoft Detours 的代码与数据节，说明 Detours 被静态链接进了主程序。清单要求 `asInvoker`，不请求管理员权限。

调试目录保留原始 PDB 名 `obs64.pdb`，`obs.dll` 保留 `obs.pdb`，`obs-frontend-api.dll` 保留 `obs-frontend-api.pdb`——这是本报告能复原目录树的前提。

### 2.2 原工程目录树（PDB 证据）

`obs64.pdb` 中共有 **2,101 条不重复源文件路径**；与 `obs.pdb` 合并去重、再去掉 Windows SDK 头文件后，属于 OBS 仓库的有 **1,305 条**。构建根目录是：

```text
D:\a\obs-studio\obs-studio          ← 源码根（GitHub Actions 的 D:\a 工作目录）
D:\a\obs-studio\obs-studio\build_x64 ← 树外构建目录，Release 配置
```

`D:\a\...` 是 GitHub Actions Windows runner 的标准工作目录，因此可以判断这批二进制由 CI 构建（推断，非直接观察）。

仓库内路径分布：

```text
obs-studio/
├─ frontend/                 354
│  ├─ utility/               101   输出、更新、OAuth、崩溃处理、主题、缩略图
│  ├─ components/             93   场景树、源树、音量表、Multiview、各类工具栏
│  ├─ widgets/                49   OBSBasic 主窗口的 40 余个 partial 实现
│  ├─ dialogs/                36   属性、滤镜、变换、统计、关于、缺失文件
│  ├─ oauth/                  12   Twitch / YouTube / Restream 授权
│  ├─ settings/               11   设置页与热键控件
│  ├─ wizards/                11   自动配置向导
│  ├─ docks/                   8   浏览器、YouTube 停靠面板
│  ├─ plugin-manager/          8   插件管理
│  ├─ importer/ + importers/  12   Classic / Studio / SLOBS / XSplit 导入
│  ├─ models/                  4   Rect、SceneCollection
│  ├─ api/                     2
│  └─ OBSApp、OBSStudioAPI、obs-main.cpp、updater 等根文件
├─ libobs/                   192
│  ├─ util/                   64   dstr、序列化、线程、配置、UTF-8、Win 管道
│  ├─ graphics/               42   effect、shader、纹理、GIF、数学
│  ├─ media-io/               19   音视频帧、格式、时间戳
│  ├─ callback/                8   信号/回调
│  ├─ audio-monitoring/        4
│  └─ obs-*.c / obs-*.h       55   source、scene、output、encoder、service、data…
├─ shared/                    48
│  ├─ qt/                     38   Qt 通用控件（Row、ToggleSwitch、SpinBox…）
│  ├─ properties-view/         7   属性编辑器
│  └─ bpm/                     3
├─ deps/                      23   libcaption、blake2、json11、w32-pthreads
├─ plugins/                    1   obs-browser
├─ .deps/                    497   CI 依赖包（见 2.3）
└─ build_x64/                190   生成物（见 2.4）
```

`frontend/widgets` 里的 `OBSBasic_Scenes.cpp`、`OBSBasic_Streaming.cpp`、`OBSBasic_Recording.cpp`、`OBSBasic_StudioMode.cpp`、`OBSBasic_Projectors.cpp`、`OBSBasic_VirtualCam.cpp` 等文件说明：主窗口不是一个大文件，而是**一个类按功能拆成 40 多个 `.cpp`**。这是 OBS 前端最显著的工程组织方式。

`frontend/utility` 则集中了“非界面”的前端逻辑：`SimpleOutput` / `AdvancedOutput` / `FFmpegCodec` / `FFmpegFormat` / `MultitrackVideoOutput` / `WHIPSimulcastEncoders` 负责输出，`AutoUpdateThread` / `update-helpers` 负责更新，`CrashHandler_Windows` 负责崩溃捕获，`OBSTranslator` / `OBSTheme` 负责语言与主题，`ThumbnailManager` 负责缩略图。

### 2.3 依赖与工具链

PDB 中出现的依赖目录是：

```text
.deps/obs-deps-2026-10-23-x64        158 条头文件路径
.deps/obs-deps-qt6-2026-10-23-x64    339 条头文件路径
```

按头文件统计的依赖内容：

| 依赖 | 证据 |
| --- | --- |
| Qt 6 | `QtCore` 190、`QtWidgets` 69、`QtGui` 66、`QtNetwork` 7、`QtXml` 4、`QtSvg` 3 |
| FFmpeg 62 | `libavutil` 22、`libavcodec` 9、`libavformat` 4、`libavdevice` 3、`libswresample` 3、`libswscale` 3 |
| mbedTLS / PSA | `mbedtls` 32、`psa` 21 |
| curl | `curl` 10 |
| nlohmann/json | `nlohmann` 45 |
| Detours | `detours.h` |
| jansson | `jansson.h` |
| zlib | `zlib.h` / `zconf.h` |

导入表进一步确认运行时依赖：`Qt6Widgets/Gui/Core/Network/Svg/Xml`、`avcodec-62`、`avformat-62`、`libcurl`、`w32-pthreads`、`obs.dll`、`obs-frontend-api.dll`、`dxgi`、`dwmapi`、`AVRT`、`WINMM`、MSVC 运行库。

`obs.dll` 还导入 `swscale-9`、`swresample-6`、`avutil-60`、`zlib`、`UuidCreate`，说明**缩放、重采样、音频时钟和 UUID 都在 libobs 内部完成**，不是前端临时调用。

### 2.4 CMake + Qt 的构建模型

`build_x64` 中的生成物是 CMake + Qt 自动生成的直接证据：

```text
build_x64/frontend/obs-studio_autogen/
├─ include_Release/…/moc_*.cpp             Qt AUTOMOC
├─ include_Release/…/ui_*.h                Qt AUTOUIC
├─ QM7UNKGVG2/qrc_obs_Release.cpp          Qt AUTORCC
└─ mocs_compilation_Release.cpp            合并的 moc 编译单元
build_x64/config/…                          生成的配置头
build_x64/shared/qt/…                       共享 Qt 控件的 moc
```

目标名可以从 autogen 目录名确定为 **`obs-studio`**，配置为 **Release**，构建方式是**树外构建**（源码在 `obs-studio/`，生成物在 `build_x64/`）。`moc_OBSApp.cpp`、`moc_OBSBasic.cpp`、`moc_properties-view.cpp`、`moc_browser-panel.cpp` 等文件名同时证明这些类使用了 `Q_OBJECT`。

发行目录也符合 Qt 部署布局：`platforms/qwindows.dll`、`imageformats/qjpeg|qsvg|qwebp…`、`styles/qmodernwindowsstyle.dll`、`iconengines/qsvgicon.dll`。

### 2.5 前端架构

入口与分层可以从源文件名直接读出：

```text
frontend/obs-main.cpp         程序入口
frontend/OBSApp.{cpp,hpp}     QApplication 子类，负责主题、语言、样式
frontend/OBSApp_Themes.cpp    主题加载
frontend/OBSStudioAPI.{cpp,hpp}  obs_frontend_callbacks 的实现
frontend/widgets/OBSBasic.*   主窗口
frontend/api/                 obs-frontend-api 的头文件侧
```

`obs-frontend-api.dll` 只有 41 KB，导出 **100 个函数**，是“前端能力”的稳定 ABI。按功能分组：

| 分组 | 代表导出 |
| --- | --- |
| 场景与切换 | `obs_frontend_get_scenes`、`get_current_scene`、`set_current_scene`、`get_current_preview_scene`、`preview_program_mode_active`、`trigger_transition` |
| 转场 | `get_transitions`、`get/set_current_transition`、`get/set_transition_duration`、`get/set_tbar_position`、`release_tbar` |
| 输出 | `recording_start/stop/active/pause`、`streaming_start/stop/active`、`replay_buffer_*`、`virtualcam_*`、`take_screenshot` |
| 配置 | `get_app/global/user/profile_config`、`get_profiles`、`create/delete/duplicate_profile`、`save` |
| 界面 | `get_main_window`、`get_main_window_handle`、`add_dock_by_id`、`add_tools_menu_item`、`open_projector`、`open_source_properties/filters/interaction` |
| i18n | `push_ui_translation`、`pop_ui_translation`、`get_locale_string` |
| 插件扩展 | `add_event_callback`、`add_save_callback`、`add_preload_callback`、`add_undo_redo_action`、`add_scene_collection` |

`obs64.exe` 从 `obs.dll` 导入了 **776 个符号**。按子系统统计，能看出前端把精力放在哪里：

| 前缀 | 导入数 | 含义 |
| --- | ---: | --- |
| `obs_source_*` | 99 | 源的生命周期、属性、滤镜、音量 |
| `gs_*` | 59 | 直接调用图形子系统（预览、项目器） |
| `obs_sceneitem_*` | 58 | 场景项的变换、裁剪、排序、可见性 |
| `obs_data_*` | 48 | 设置数据 |
| `obs_property_*` | 48 | 属性界面 |
| `os_*` | 41 | 平台抽象 |
| `obs_output_*` | 39 | 输出控制 |
| `obs_get_*` | 39 | 全局枚举 |
| `obs_encoder_*` | 26 | 编码器 |
| `config_*` | 25 | 配置存取 |
| `obs_hotkey_*` | 24 | 热键 |
| `obs_service_*` | 20 | 推流服务 |
| `obs_scene_*` | 19 | 场景 |
| `obs_display_*` / `obs_view_*` / `obs_canvas_*` | 26 | 显示与画布 |

这组数字说明：OBS 前端本质上是一个**围绕 `obs_source` / `obs_sceneitem` 的 Qt 编辑器**，图形与合成仍然由 libobs 完成。

### 2.6 libobs 核心的公开面

`obs.dll` 导出 **1,781 个符号**，按前缀分组：

| 前缀 | 导出数 |
| --- | ---: |
| `gs_*` | 281 |
| `obs_source_*` | 156 |
| `obs_data_*` | 156 |
| `os_*` | 109 |
| `obs_property_*` | 81 |
| `obs_output_*` | 75 |
| `obs_sceneitem_*` | 67 |
| `obs_encoder_*` | 61 |
| `obs_get_*` | 60 |
| 数学（`vec*` / `matrix*` / `quat*`） | 54 |
| `obs_hotkey_*` | 36 |
| `config_*` | 32 |
| `dstr_*` | 29 |
| `obs_properties_*` | 28 |
| `video_*` | 27 |
| `obs_canvas_*` | 26 |
| `obs_service_*` | 26 |
| `obs_transition_*` | 25 |
| `obs_weak_*` | 25 |
| `obs_scene_*` | 24 |
| `profiler_*` | 24 |
| `obs_enum_*` | 21 |

`libobs/` 的核心文件也印证了这个分层：`obs-source.c`、`obs-scene.c`、`obs-output.c`、`obs-encoder.c`、`obs-service.c`、`obs-data.c`、`obs-properties.c`、`obs-hotkey.c`、`obs-canvas.c`、`obs-view.c`、`obs-display.c`、`obs-module.c`，以及 `graphics/` 的 effect / shader / texture 实现、`media-io/` 的帧与格式、`util/` 的容器与平台层。

图形后端独立成 DLL：`libobs-d3d11.dll`、`libobs-opengl.dll`、`libobs-winrt.dll`。这意味着原工程把“图形 API”做成了可替换模块，而不是编译期写死。

### 2.7 插件与发行布局

OBS 安装包共 **2,304 个文件**，其中 27 个核心模块位于 `core/<module>/`：

| 模块 | DLL |
| --- | --- |
| aja | `aja.dll` |
| aja-output-ui | `aja-output-ui.dll` |
| coreaudio-encoder | `coreaudio-encoder.dll` |
| decklink | `decklink.dll` |
| decklink-captions | `decklink-captions.dll` |
| decklink-output-ui | `decklink-output-ui.dll` |
| frontend-tools | `frontend-tools.dll` |
| image-source | `image-source.dll` |
| nv-filters | `nv-filters.dll` |
| obs-browser | `obs-browser.dll` + CEF（`libcef.dll`、`chrome_elf.dll`、`libEGL.dll`、`libGLESv2.dll`、`dxcompiler.dll`、`dxil.dll`） |
| obs-ffmpeg | `obs-ffmpeg.dll` |
| obs-filters | `obs-filters.dll` |
| obs-nvenc | `obs-nvenc.dll` |
| obs-outputs | `obs-outputs.dll` |
| obs-qsv11 | `obs-qsv11.dll` |
| obs-text | `obs-text.dll` |
| obs-transitions | `obs-transitions.dll` |
| obs-vst | `obs-vst.dll` |
| obs-webrtc | `obs-webrtc.dll` |
| obs-websocket | `obs-websocket.dll` |
| obs-x264 | `obs-x264.dll` |
| rtmp-services | `rtmp-services.dll` |
| text-freetype2 | `text-freetype2.dll` |
| vlc-video | `vlc-video.dll` |
| win-capture | `win-capture.dll` + `graphics-hook32.dll` / `graphics-hook64.dll` |
| win-dshow | `win-dshow.dll` + `obs-virtualcam-module32.dll` / `64.dll` |
| win-wasapi | `win-wasapi.dll` |

`bin/64bit/` 里还有几个辅助可执行文件：`obs-ffmpeg-mux.exe`（FFmpeg 复用器子进程）、`obs-amf-test.exe`、`obs-nvenc-test.exe`、`obs-qsv-test.exe`（硬件编码自检）、`obs-scripting.dll` + `lua51.dll`（脚本）、`librist.dll` / `srt.dll` / `datachannel.dll`（传输）。

`data/obs-studio/` 是数据根：183 个主题文件、77 个语言文件、`OBSPublicRSAKey.pem`、`locale.ini`、`authors`、`license`。这说明 OBS 的“主题 / 语言 / 许可”都是**外部数据**，不是编译进 exe 的资源。

### 2.8 OBS 原工程复原度

| 项目 | 复原度 | 依据 |
| --- | --- | --- |
| 源码目录树 | 高 | PDB 中 1,305 条仓库内路径 |
| 构建系统与目标名 | 高 | `obs-studio_autogen`、`mocs_compilation_Release.cpp` |
| 依赖及版本目录 | 高 | `.deps/obs-deps-2026-10-23-x64`、`.deps/obs-deps-qt6-2026-10-23-x64` |
| 模块边界 | 高 | 前端 API 100 导出、libobs 1,781 导出、27 个插件 DLL |
| 文件级实现 | 中 | 有源文件名，没有函数体；原生反编译不可用 |
| CMakeLists 原文 | 低 | 只能从生成物反推目标与 AUTOMOC/AUTOUIC/AUTORCC |
| Git 提交号 | 无 | PDB 不含 commit 信息 |

---

## 3. vMix 29（附 vMix 28 对照）

### 3.1 打包与程序集身份

vMix 29 安装包是 **Inno Setup 6.1.0 Unicode**，481,764,296 B。只读解包得到：

- `app/vMix.exe`：31,471,096 B，程序集 `vMix, Version=29.0.0.50`
- `app/languages/zh.txt`：95,288 B，1,297 条 / 267 个窗体前缀

`vMix.exe` 的 CLR 头：

```text
PE32 / x86 / AnyCPU / IL-only
Runtime version : v4.0.30319
TargetFramework : .NETFramework,Version=v4.0
MVID            : 63c8ce26-7389-41f6-83e6-83b406ee8670
```

它引用 `Microsoft.VisualBasic 10.0.0.0`、`System.Windows.Forms`、`System.Drawing`，窗体统一以 `frm` 前缀命名（`frmmain`、`frmInput`、`frmSettings`…），可以判定原工程是 **VB.NET + WinForms**（观察 + 推断）。同时引用了 `PresentationCore` / `PresentationFramework` / `WindowsBase` / `System.Xaml`，说明部分界面或控件使用 WPF 互操作（推断）。

### 3.2 28 与 29 的元数据对照

| 指标 | vMix 28.0.0.42 | vMix 29.0.0.50 |
| --- | ---: | ---: |
| 文件大小 | 33,248,256 B | 31,471,096 B |
| TypeDef | 5,310 | 5,218 |
| MethodDef | 37,804 | 31,958 |
| Field | 15,875 | 16,172 |
| MemberRef | 6,214 | 5,937 |
| AssemblyRef | 43 | 50 |
| ModuleRef | 33 | 36 |
| ManifestResource | 158 | 152 |
| CustomAttribute | 6,041 | 6,176 |
| `frmmain` 字段 | 186 | 190 |
| `frmmain` 方法 | 515 | 522 |

29 版的方法数明显少于 28 版，但字段和窗体成员略有增加。仅凭静态元数据无法判断这是重构、裁剪还是混淆策略变化；这里只记录观察，不做因果推断。

### 3.3 模块地图（来自 50 条 AssemblyRef）

vMix 的程序集引用本身就是一张原工程模块图：

| 层 | 程序集 | 作用（按名称与引用关系推断） |
| --- | --- | --- |
| 运行时 / UI | `mscorlib`、`Microsoft.VisualBasic`、`System.Windows.Forms`、`System.Drawing`、`System`、`System.Core`、`System.Xml`、`System.Data`、`System.Configuration`、`System.Management` | .NET Framework 4.0 与 WinForms 基础 |
| WPF 互操作 | `PresentationCore`、`PresentationFramework`、`WindowsBase`、`System.Xaml` | 部分控件/渲染 |
| 文本 | `C1.Win.C1SpellChecker.2` 2.0.20181.296、`vMixTitleLibrary` | 拼写检查、标题 |
| 采集 | `DirectShowLib-2005`、`DirectShowLib.Utils`、`WindowsMediaLib`、`MediaFoundation`、`VCMWrapper` | DirectShow、Media Foundation、视频采集 |
| 图形 | `SlimDX 4.0.13.43`、`GraphicsUI`、`GraphicsEngine`、`D2DEngine` | Direct3D / Direct2D 封装与渲染 |
| 编码 / 媒体 | `AVSDK`、`vMixCodecLibraryNet 4.0.0.15`、`vMixDesktopCaptureLibrary 29.0.0.3` | 编解码、桌面采集 |
| NDI / OMT / PTZ | `NDIInterop`、`NDIPluginAPI`、`libomtnet 1.0.0.20`、`PTZProvider`、`vMixZoomNodeInterop` | NDI、OMT、PTZ、Zoom 集成 |
| 应用 / 集成 | `vMixAPI`、`vMixInterop`、`vMixStreamDeckLibrary`、`DataSourceAPI` | 外部 API、Stream Deck、数据源 |
| 其他 | `PhoenixLibrary 3.0.0.18`、`PhoenixInstance3`、`System.Speech`、`System.Web`、`System.Web.Services`、`System.Runtime.Serialization` | 语音、Web、序列化 |

这解释了 vMix 为什么能以单个 `vMix.exe` 发行：**主程序集承载全部窗体和业务流程，采集/图形/编码/协议被拆成同目录下的内部程序集**。`ModuleRef` 36 条说明还有一批 P/Invoke 原生 DLL，但 REA 的托管视图没有展开模块名，这里不猜。

### 3.4 窗体清单

`zh.txt` 中出现 **267 个窗体/组件前缀**。按条目数排序的前 15 个：

| 窗体 | 条目 | 英文标题 |
| --- | ---: | --- |
| `frmSettings` | 280 | Settings |
| `frmInputSettings` | 135 | Input Settings |
| `frmInput` | 134 | Input Select |
| `ToolTip` | 80 | — |
| `frmAudioSettings` | 63 | Audio Settings |
| `frmRecordingSetup` | 40 | Recording Setup |
| `frmmain` | 38 | — |
| `frmShortCut` | 37 | Add Shortcut |
| `frmOverlay` | 36 | Overlay Settings |
| `frmMultiCorder` | 24 | MultiCorder |
| `frmStreaming` | 21 | Streaming Settings |
| `frmRecordingInfo` | 16 | Recording Information |
| `frmTextEditor` | 14 | Title Editor |
| `frmVirtualSet` | 14 | Virtual Set Zoom Editor |
| `frmPlaylist` | 13 | PlayList |

这个分布很能说明 vMix 的产品形态：**主窗体只有 38 条文案，设置与输入配置却占了几百条**。大量能力被折叠进对话框，主界面只保留高频操作。

`vMix.frmmain` 在 29 版有 **190 个字段、522 个方法**。28 版恢复出的原始 VB 控件名包括 `pMain`、`pPreview`、`cmdCut`、`cmdFade`、`cmdFade2`、`cmdFade4`、`cmdEffect`、`cmdFTB`、`cmdAddInput`、`cmdOverlaySettings`、`cmdRecStartStop`、`cmdStreaming`、`cmdAudioMixer` 等——这与语言包中的主窗体条目一致。

### 3.5 资源清单

152 条 ManifestResource 中，体量最大的是：

| 资源 | 大小 | 说明 |
| --- | ---: | --- |
| `vMix.Resources.resources` | 21,421,762 B | 主资源包，占整个 exe 的 68% |
| `vMix.frmInput.resources` | 148,560 B | 输入选择窗体的图标/位图 |
| `vMix.frmSplash.resources` | 42,955 B | 启动画面 |
| `vMix.frmTextEditor.resources` | 32,827 B | 标题编辑器 |
| `vMix.frmTallyLight.resources` | 29,629 B | Tally 灯 |
| `vMix.frmmain.resources` | 28,786 B | 主窗体 |
| 其余窗体 `.resources` | 约 23,308 B / 个 | 其中多数哈希相同，说明共用同一份图标资源 |

资源里还能直接看到产品用到的 Web 资产：

```text
vMix.telestrator.css / .htm / .js
vMix.websocketvideo.js
vMix.bluezone.png / vMix.orangezone.png
vMix.zero64.png … vMix.twenty64.png
vMix.fullscreen64.png / vMix.undo64.png / vMix.image64.png / vMix.pointer64.png
vMix.greentick.svg / vMix.redcross.svg / vMix.thoughtbubbleleft.svg / vMix.thoughtbubbleright.svg
vMix.exe.licenses
```

`vMix.Resources.resources` 的 21 MB 与 `vMix.exe.licenses` 说明 vMix 把**图标、声音、Web 页面、第三方许可**全部嵌入主程序集，而不是放在外部数据目录。这与 OBS 的 `data/obs-studio` 外部数据布局正好相反。

### 3.6 混淆与不可恢复项

vMix 29 的 `frmmain` 方法表里可以直接看到：

```text
get_zzzzzzzzk
set_zzzzzzzzk
get_zzzzzzzzzzzzzzzk
set_zzzzzzzzzzzzzzzk
get_zzzzzzzzzzzzzzzl
```

资源名也出现 `jzzzzzb.resources`、`pzzzzza.resources`、`qzzzzzm.resources`、`rzzzzzb.resources` 这类模式。28 版的 `InitializeComponent` 被 REA 观察为 4/6 字节桩。这些都与 Eziriz .NET Reactor 的改名与方法来保护特征一致（28 版结论为观察，29 版为同类特征推断）。

因此以下内容**无法**从二进制恢复：

- `.sln` / `.vbproj` 的工程文件与编译顺序
- 原始 VB 源文件名、` designer.vb` 内容
- 窗体精确坐标、字体、控件尺寸（`InitializeComponent` 被保护）
- 业务方法的真实 IL（部分被加密/桩化）
- 36 个 `ModuleRef` 对应的原生 DLL 名称

能恢复的是**模块边界、窗体清单、资源清单、主窗体成员规模、依赖版本**，这些足以复原“原工程长什么样”，但不足以还原“每一行怎么写”。

---

## 4. 两个原工程的组织差异

| 维度 | OBS Studio 33.0.3 | vMix 29 |
| --- | --- | --- |
| 语言 / 运行时 | C / C++17，MSVC，原生 | VB.NET，.NET Framework 4.0，IL-only |
| UI 框架 | Qt 6 Widgets | WinForms（局部 WPF） |
| 构建 | CMake + Ninja/MSBuild，树外 `build_x64` | 未保留；推断为 Visual Studio + VB.NET 项目 |
| 模块化 | 进程内 libobs + 27 个插件 DLL + 图形后端 DLL | 单主程序集 + 20 多个内部程序集 |
| 扩展方式 | `obs-frontend-api` 100 个 C 导出 + 模块加载 | `vMixAPI` / `vMixInterop` / Stream Deck / 数据源 |
| 数据与资源 | 外部 `data/obs-studio`（主题、语言、许可） | 嵌入 `vMix.Resources.resources`（21 MB） |
| 国际化 | 77 个 locale 文件，运行时可切换 | 267 个窗体前缀的语言包，含机器翻译 |
| 可复原度 | 高（PDB 带源文件路径） | 中（元数据完整，代码被混淆） |

对 Mixvera 的直接启示不是“照抄”，而是两条已经被两家验证过的工程原则：

1. **核心与界面分离**：OBS 用 `libobs` + `frontend` 的硬边界，vMix 用 `GraphicsEngine` / `AVSDK` / `vMixAPI` 等程序集边界。Mixvera 现有的 `core` / `services` / `engine` / `ui` 分层与这条原则一致。
2. **主界面保持克制**：vMix 主窗体只有 38 条文案，设置与输入配置外移到对话框；OBS 主窗口也把场景、源、混音器、输出拆成独立面板。Mixvera 0.3 的 vMix 式布局应继续把“高频操作”留在主界面，把参数收进对话框。

---

## 5. 复现方式

```powershell
# OBS：只读列出安装包内容（不运行安装器）
& 'C:\Users\CYH\AppData\Local\JetBrains\Toolbox\bin\7z.exe' l -tNsis `
  'OBS-Studio-33.0.3-Windows-x64-Installer.exe'

# OBS：只读提取主程序与 PDB
& 'C:\Users\CYH\AppData\Local\JetBrains\Toolbox\bin\7z.exe' x -tNsis -y `
  -obuild\analysis\obs-app 'OBS-Studio-33.0.3-Windows-x64-Installer.exe' `
  'bin\64bit\obs64.exe' 'bin\64bit\obs.dll' 'bin\64bit\obs64.pdb' 'bin\64bit\obs.pdb' `
  'bin\64bit\obs-frontend-api.dll' 'bin\64bit\obs-frontend-api.pdb'

# OBS：PDB 源文件路径
python -X utf8 -I build\analysis\scan_pdb.py `
  build\analysis\obs-app\bin\64bit\obs64.pdb --paths

# OBS：PE 导入 / 导出 / 版本 / 清单
python -X utf8 -I build\analysis\pe_report.py `
  build\analysis\obs-app\bin\64bit\obs64.exe `
  build\analysis\obs-app\bin\64bit\obs.dll `
  build\analysis\obs-app\bin\64bit\obs-frontend-api.dll

# vMix 29：只读解包（innoextract，不运行安装器）
innoextract -e -d build\analysis\vmix29 vmix29.exe
```

REA 侧的调用与证据 ID 见下表。所有 REA 调用都是静态检查，没有加载 CLR 或执行目标代码。

## 6. 证据索引

| 证据 | ID |
| --- | --- |
| OBS `obs64.exe` PE 资源树 | `ev_5348716a74b7a64315aefecef07dd0540867e2eed6f47dd49b3b216b650b1dd3` |
| OBS 文件识别（Qt / x86-64 / 9,514 函数） | `ev_1679bb7a513a750f12b9fc26e0eb02d4dba2ae961869de1b8bccba46858ac809` |
| OBS 概览（26,102 字符串 / 3,655 导入） | `ev_f255d10c01c2947186b061521e6be42663da39944108737dde3c728e2e9ab5c1` |
| vMix 29 程序集摘要 | `ev_d196cc18fa436df5f76b16c7700a32195335d4663054c0b0fc9d3da93d724ed0` |
| vMix 29 `frmmain` 成员摘要 | `ev_01749ac9d721275fdd2d69cc6492423a1fd05aa72426dbc4b793a63ecc386834` |
| vMix 29 程序集引用（50 条） | `ev_406459bf27b359378309b53bef894eaf53ad8723738b43ddf07d02425134e1c9` |
| vMix 29 资源清单（152 条） | `ev_2a0d4a4ddf00d7aa583a0712534b944004cb1568b8c62d30d30e55d35c86f255` |
| vMix 28 程序集摘要 | `ev_6829731fba8248886c36c95cf9d971299cfefde9025da3f0a3dec5b2babc910d` |
| vMix 28 `frmmain` 成员摘要 | `ev_e4dfc8a277a10f4df6b15a1e3328f36790f53811ec80f8a9b42f407ff2351ebd` |

## 7. 已知限制

- 没有运行 OBS 或 vMix 的安装器，也没有启动 `obs64.exe` / `vMix.exe`；所有结论来自文件静态结构。
- REA 原生 provider（Ghidra 12.1.4 已安装但未配置到本会话）不可用，OBS 没有函数级反编译；结论来自 PDB、PE 导入导出、资源与字符串。
- OBS PDB 提供源文件路径，但不含源码内容、编译参数和 commit。
- vMix 的 `InitializeComponent` 与部分方法被 .NET Reactor 保护，窗体精确几何、字体和控件尺寸不可恢复。
- vMix 29 的 36 条 `ModuleRef` 未展开，原生 DLL 依赖只按程序集引用推断到模块级。
- 本报告只描述结构与元数据，不复制任何 OBS / vMix 的代码、图标、语言包文本或窗体布局坐标。
