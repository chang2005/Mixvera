# Mixvera 基础架构

当前版本为 `0.1.0` 基础框架，采用“服务化内核 + Qt 前端 + 可替换媒体引擎”的三层结构。

## 目录

```text
apps/mixvera        应用入口
src/mixvera/core    事件总线、服务容器、应用生命周期和领域模型
src/mixvera/services 输入、切换、转场、音频、输出、回放和设备服务
src/mixvera/control 远程控制协议骨架
src/mixvera/engine  媒体引擎接口与 Stub 后端
src/mixvera/ui      Qt Widgets 前端
tests/core          核心服务单元测试
third_party         第三方单头文件依赖
```

## 分层规则

1. UI 不得直接持有 libobs 对象；只能通过服务层访问状态和操作。
2. 服务之间通过事件总线解耦；同步查询通过服务容器完成。
3. `MediaEngine` 是媒体后端的唯一抽象点。当前使用 `StubMediaEngine`，后续新增 `LibobsMediaEngine`。
4. 内部控制和外部控制共用领域服务，不会让远程协议成为实时切换主链路。
5. 当前 `control/control_server.cpp` 仅提供 JSON-RPC 请求处理骨架，尚未绑定网络传输。

## 后续接入 libobs

构建时设置 `-DMIXVERA_ENABLE_LIBOBS=ON` 并提供 `libobsConfig.cmake`。业务服务接口保持不变，引擎实现负责：

- `obs_init` / `obs_startup` 生命周期；
- source、canvas、view 的创建和销毁；
- T-Bar 到 `obs_transition_set_manual_time()` 的映射；
- 多输出 `obs_output_t` / encoder / service 的绑定；
- 6 路 mixer mask 与音频服务状态同步。

## 许可注意

Mixvera 的核心若链接 libobs，分发时需遵守 GPL-2.0-or-later。第三方依赖的许可与归属声明应继续维护在 `THIRD_PARTY_NOTICES.md`。
