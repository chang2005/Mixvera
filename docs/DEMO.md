# Mixvera 0.2 使用说明

这一版已经可以当真正的导播台用：接真实画面、编场景、切换、录制。

![Mixvera](demo/mixvera-0.2.png)

## 1. 第一次启动

启动后是一个「Welcome」工程，已包含：

- `Camera 1`：活动测试图案（带扫描光带与时码）
- `Camera 2`：纯色垫场
- `Mixvera Title`：标题图层
- `Scene 1`：测试图案 + 标题
- `Scene 2`：纯色 + 半透明标题

PGM 显示 Scene 1，PVW 显示 Scene 2，可以直接按 `Space` 看一次硬切，或 `Enter` 看一次 500 ms 淡入淡出。

## 2. 添加真实输入

点顶栏 **Add Input**（或 `Ctrl+I`），选择类型：

| 类型 | 需要选择的参数 |
| --- | --- |
| Screen Capture | 显示器序号（对话框会列出分辨率） |
| Window Capture | 窗口标题（可刷新列表） |
| Camera | 摄像头设备 |
| Image | 图片文件（png / jpg / bmp / webp / gif） |
| Media File | 视频或音频文件（循环播放） |
| Title | 文本、字号、对齐、颜色 |
| Colour | 颜色 |
| Test Pattern | SMPTE 彩条 |

新输入会自动加入当前场景，并出现在右侧输入列表里。

> 屏幕采集抓的是整个桌面。把 Mixvera 放在前面时会看到「画中画」递归效果，这是正常的。

## 3. 组织场景

左侧 **SCENES**：

- `+` 新建场景，`⧉` 复制，`−` 删除
- 单击 = 设为预览（PVW），双击 = 直接切到节目（PGM）
- 右键可重命名 / 复制 / 删除

**SOURCES** 是当前场景的图层（从下到上）：

- 勾选框控制该图层显隐
- 右键可上移 / 下移 / 从场景移除
- `+` 从已有输入里挑一个加入当前场景

## 4. 切换与转场

- **CUT**（`Space`）：直接切
- **AUTO**（`Enter`）：按当前转场类型和时长自动完成
- **Fade / Wipe / Slide / Zoom**：四种转场
- **T-BAR**：手动推子，推过 50% 松手完成，不到 50% 自动回退
- **FTB**：淡出到黑

PGM 监视器在转场过程中会显示混合画面与 `MIX xx%` 读数。

## 5. 音频

底部混音器：

- 带音频的输入（屏幕 / 窗口 / 摄像头 / 视频 / 音频文件）会出现通道
- 推子调节增益（-60…+12 dB），`M` 静音
- 主输出电平表由系统默认录音设备驱动；没有设备时显示 `NO AUDIO INPUT`

## 6. 录制与推流

- **RECORD**：开始录制，顶栏 `REC` 胶囊开始计时，再点一次停止
- **STREAM**：按设置里的 RTMP 地址推流，顶栏 `STREAM` 胶囊计时

两者都通过 ffmpeg 的原始帧管道完成，需要 `ffmpeg` 在 `PATH` 中。路径与推流地址在 **Settings** 里配置：

- 录制目录留空 → `%USERPROFILE%/Videos/Mixvera`
- 推流 URL 形如 `rtmp://live.example.com/app`，串流密钥单独填写，启动时会拼到末尾

录制文件名为 `Mixvera_yyyyMMdd_HHmmss.mp4`。

![屏幕采集示例](demo/mixvera-screen.png)

## 7. 工程与快捷键

工程自动保存在 `%APPDATA%/Mixvera/project.json`，退出自动写回，启动自动恢复。用 `File → Save Project As…` 可另存，`File → Open Project…` 可打开，也可以用命令行：

```powershell
.\Mixvera.exe --project D:\shows\demo.json
```

| 按键 | 动作 |
| --- | --- |
| `Space` | CUT |
| `Enter` | AUTO |
| `F11` / `Esc` | 全屏 / 退出全屏 |
| `Ctrl+R` | 录制启停 |
| `Ctrl+I` | 添加输入 |
| `Ctrl+S` / `Ctrl+O` | 保存 / 打开工程 |

## 8. 常见问题

**启动后黑屏 / 录像没有画面**
检查监视器是否显示 `NO SIGNAL`。摄像头与视频文件需要 Qt Multimedia 后端；若日志出现 `No QtMultimedia backends found`，说明 exe 同级缺少 `multimedia/` 插件目录，执行：

```powershell
D:\Qt\6.8.3\mingw_64\bin\windeployqt.exe --release --no-translations --compiler-runtime build\mingw\Mixvera.exe
```

**点 RECORD 报找不到 ffmpeg**
把 ffmpeg 加入 `PATH`，或在 `Settings` 里确认提示行显示了 ffmpeg 路径。仓库不内嵌 ffmpeg。

**截图模式**

```powershell
.\Mixvera.exe --no-restore --screenshot out.png --screenshot-delay 2000
.\Mixvera.exe --no-restore --dump-ui ui-tree.txt
```

`--dump-ui` 输出控件树（类名、对象名、几何、文本），用于无视觉环境下的布局核对。