# Changelog

## [0.6.0] — 2026-11-25

把芯象（SinSam）26.7.10.3 的**竖屏 / 显示模式、蓝绿幕抠像与字幕叠加**，以及 vMix GT / SinSam 的**标题包装**移植到 Mixvera。0.5 的叠加层、转场与标题全部沿用；取证与兼容矩阵见 `docs/RE-SINSAM.md`。

### 新增

- **竖屏 / 方形画布**：设置 → 视频新增 `1080 × 1920`（Portrait FHD）、`720 × 1280`（Portrait HD）、`1080 × 1080`（Square）预设；`--canvas WxH` 启动参数；画布尺寸同步到合成器与所有标题渲染，录制、推流与快照使用同一尺寸
- **显示模式**：设置 → 视频的画布卡片新增「显示模式」下拉（横屏 / 竖屏），与预设、宽高、像素比联动——方形画布切换模式时自动展开为 16:9；「视图 → 横屏模式 / 竖屏模式」互斥切换，`--mode landscape|portrait` 可在启动时指定
- **蓝 / 绿幕抠像**：输入级实时色度键控，颜色支持绿幕、蓝幕与任意自定义色，参数为相似度 / 平滑度 / 溢色抑制；添加输入对话框新增「CHROMA KEY」面板，输入条右键「抠像」可快速切换关 / 绿幕 / 蓝幕；设置随工程保存
- **抠像管线**：在每一帧采集之后、进入场景合成之前完成，输出预乘 ARGB；叠加层、转场、录制、推流与快照都得到抠像后的画面
- **字幕条样式**：标题新增「字幕条」——底部安全区半透明底板、居中自动换行文字、强调色下划线，竖屏画布下按宽度缩放并贴底
- **滚动字幕样式**：底部滚动条，文字从右向左无缝循环，速度 20–800 px/s 可调
- **字幕文案切换**：字幕 / 滚动字幕输入可存多行预置文案，输入条右键「设置字幕文字…」「下一句」「上一句」「清除字幕」实时切换
- **画布感知标题**：`SourceProvider::setCanvasSize(QSize)` 把画布尺寸下发给全部标题，字体、边距、卡片与滚动速度按画布宽度缩放
- **标题包装**：添加输入 → 标题新增第七种样式「标题包装」。可读取 vMix / SinSam 的 `.gtzip`（zip 内 `document.xml` + `resources.xml`）、未压缩的包装目录，以及散装 `.xaml` / `.sinsamtitle` 文档；UTF-8 BOM 与声明不一致、缺省 `Color`、缺省 `Duration` 等真实模板写法都能容忍
- **元素渲染**：`Composition` / `Layer` / `Rectangle` / `Ellipse` / `Triangle` / `RightTriangle` / `TextBlock` / `Image` / `Ticker` / `QRCode`；画笔支持 `Solid` / `LinearGradient` / `RadialGradient` / `Transparent` / `Bitmap`，渐变按 `StartPoint` / `EndPoint` 与色标位置绘制，缺省颜色按透明处理
- **GT 几何**：`Dimensions` / `Location` / 圆角 / 描边 / `Opacity` / `Visible`；`Transform Rotate="x,y,z"` 取 z 分量（弧度，>6.5 视为角度）；图片 `Stretch` / `SizeMode="Stretch"` 映射为铺满，其余按等比适配
- **Storyboard 动画**：入口（`Type=""`）、`TransitionOut`、`DataChangeIn` / `DataChangeOut`；动画类型 Fade / Fly / Reveal / Expand / ZoomFade / Zoom / Bounce / Rotate（90°），支持 Direction（四方向与四角）、Delay、Duration（缺省 0.5 s）、Reverse 与 cubic / linear 缓动；容器与叶子都参与透明度、位移、缩放、旋转与揭示裁切
- **Ticker 滚动字幕**：递归读取模板中最宽的 TextBlock 作为滚动文本；Direction 为 top / bottom / up / down / vertical 时整条旋转 −90° 沿竖条滚动，其余按水平左右滚动
- **时钟与倒计时**：`{0:...}` 字段支持 .NET 自定义格式子集（`H/h`、`m`、`s`、`f/F`、`t/tt`、`d/M/y`、引号 / 转义字面量与 `%`）；倒计时取 `|` 后的格式段（如 `{0:4:00PM|mm:ss}`），`tt` 为空时不再留下尾随空格
- **二维码**：`QRCode` 使用真实 ISO/IEC 18004 字节模式编码（纠错等级 M、版本 1–10、Reed–Solomon、格式 / 版本 BCH、8 种掩码惩罚选择、4 模块静区）；空内容绘制红色 `QR` 占位
- **竖屏包装**：GT Vertical 的 3412×1920 艺术板按高度缩放并居中裁切到竖屏画布，1080×1920 包装按宽度适配
- **标题元素编辑**：添加输入对话框「Elements…」打开 `TitleElementDialog`，列出包装中的文本 / 图片 / 颜色元素并逐项覆盖；覆盖写入 `titleText:<元素>` / `titleColor:<元素>` / `titleImage:<元素>`，随工程保存
- **命名标题预置**：每个包装可存 10 组命名快照（文本、颜色、图片全覆盖）；输入条右键「保存标题预置…」「上一个 / 下一个标题预置」「删除标题预置」，切换时写入 `titlePresetChanged` 时间戳并播放包装的 `DataChangeIn` 动画
- **原地更新**：`TitleProvider::update()` 在文本、颜色、图片、预置变化时保留入场动画、滚动字幕与包装时钟；只有样式或包装文件变化才重建 provider
- **演示素材与截图**：`docs/demo/studio-background*.png`、`docs/demo/chroma-presenter*.png`（自绘素材）；`title-package-landscape.xaml`、`title-package-portrait.xaml` 及同名 `.mixvera.json`；截图 `mixvera-0.6-zh-portrait-chroma.png`（1080×1920 中文竖屏、绿幕抠像 + 字幕条）、`mixvera-0.6-en-blue-ticker.png`（1920×1080 英文、蓝幕抠像 + 滚动字幕）、`mixvera-0.6-landscape-package.png`、`mixvera-0.6-portrait-package.png`（1080×1920 竖屏包装）与 `mixvera-0.6-settings-display-zh.png`
- 文档：更新 `README.md`、`docs/DESIGN.md`（0.6 增量）、`docs/UI-SPEC.md`（SinSam / GT 对照）、`docs/RE-SINSAM.md`（0.6 证据与兼容矩阵）、`docs/ARCHITECTURE.md`

### 变更

- `Compositor::sync()` 把画布尺寸传给每个 provider；抠像参数（`chroma*`）不参与 provider 重建判定，调参时采集源不重启
- `Compositor::refresh()` 对支持的输入统一应用抠像，`detail()` 追加键控信息
- 标题样式由四种扩展为七种：全屏 / 下三分之一 / 倒计时 / 比分牌 / **字幕条** / **滚动字幕** / **标题包装**；包装样式下字幕行改称「字幕文案」，逐句操作改称「上一句字幕 / 下一句字幕 / 清除标题文字」
- `SourceProvider::update()` 与 `Compositor::sync()`：文本、颜色、图片、预置变化时原地更新，成功时只替换 descriptor，不重启采集
- `TitlePackage::render()` 增加 `dataChangeSeconds` 参数；新增公开助手 `titlePackageClockText()`，测试可用固定时间求值
- 翻译规模 351 → 448 条，新增竖屏、显示模式、抠像、字幕、包装与预置文案，全部完成
- 版本号升至 0.6.0；窗口标题与关于对话框更新

### 测试

- `mixvera_media_tests`：竖屏画布下字幕条与滚动字幕出帧且尺寸正确、绿幕与蓝幕抠像、抠像图像在场景中正确露出下层、六种内置标题样式与 GT 包装出帧、.NET 时钟格式（`HH:mm`、`hh:mm tt`、倒计时 `|` 段）、二维码结构（21×21 `vmix.com`、三个定位符、中文扩容、超长内容为空）、DataChangeIn 开场动画、文本更新保留动画时钟
- `mixvera_ui_smoke_tests`：竖屏画布切换并回写工程、字幕「下一句 / 上一句」轮换、抠像参数经添加输入对话框往返、标题预置保存 / 上一个 / 下一个 / 删除的参数快照与清理
- 四个测试目标全部通过（core / project / ui smoke / media）

## [0.5.0] — 2026-11-25

把对 SinSam（芯象）26.7.10.3 安装包的只读逆向结果落地：四路下游叠加层、Fly / Cube 转场、标题预置（下三分之一 / 倒计时 / 比分牌）与快照。取证记录见 `docs/RE-SINSAM.md`。

### 新增

- **四路下游叠加层（Overlay 1–4）**：总线顺序与 vMix / SinSam 一致——转场之后、FTB 之前；每层可指定任意输入（含场景），场景以透明背景渲染；页脚 `OVERLAY 1–4` 按钮点击开关、右键换输入；输入条右键「叠加层」子菜单；一个输入同时只占一层；删除输入自动清层；随工程 JSON 保存与恢复
- **预览叠加**：页脚 `PVW OVL` 复选框，预览监视器可显示叠加结果
- **Fly / Cube 转场**：转场栏扩展为 2×3 六种；飞入为双卡片缩放旋转飞行，立方体为带明暗渐变的水平立方旋转
- **标题四种样式**：全屏（原有）、下三分之一（主标题 + 副标题 + 强调色条）、倒计时（大号 `MM:SS` + 进度条 + 归零变红）、比分牌（赛事名 + 主客队比分）；`AddInputDialog` 标题页新增样式、副标题、倒计时长度、赛事、队名与比分字段
- **标题实时控制**：输入条右键「标题控制」——倒计时开始 / 暂停 / 重置（真实时钟，`countdownEnd` 时间戳随工程保存）、主客队得分 ±1
- **快照**：页脚按钮、输出菜单与 `Ctrl+Shift+P`，把当前节目输出（含叠加与 FTB）保存为时间戳 PNG 到录制目录
- **逆向报告**：`docs/RE-SINSAM.md`——安装包 / 程序集 / 资源 / MCP 工具目录 / 标题模板的 SHA-256、功能地图、0.5 取舍与复现命令
- 文档：`README.md`、`docs/DESIGN.md`（0.5 增量表）、`docs/UI-SPEC.md`（SinSam 对照表）、`docs/RE-SINSAM.md` 更新；新增截图 `docs/demo/mixvera-0.5-*.png`

### 变更

- `TransitionKind` 增加 `Fly` / `Cube`，JSON 键 `fly` / `cube` 往返
- `Project` 增加恒为四条的 `overlays` 数组；加载时清理已不存在的输入引用
- `Compositor::renderScene` 支持透明背景，供叠加层复用
- 标题输入的运行参数（倒计时 end/remaining/paused）在属性对话框中保留不被覆盖
- 翻译规模 305 → 351 条，新增叠加层、标题样式与快照文案，全部完成
- 版本号升至 0.5.0；窗口标题与关于对话框更新
- QSS 新增 `overlayButton` / `footerCheck` 样式（三主题 token 驱动）

### 测试

- `mixvera_project_tests`：Fly / Cube 键往返、叠加层 JSON 往返与失效输入清理
- `mixvera_media_tests`：四种标题样式出帧且非空、倒计时 / 比分 `detail()`、叠加层像素合成（开 / 关）、Fly / Cube 转场渲染
- `mixvera_ui_smoke_tests`：叠加层按钮选输入后点亮、快照按钮写 PNG
- 四个测试目标全部通过（core / project / ui smoke / media）

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