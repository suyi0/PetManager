# Redis 广播频道统一订阅总线（RedisMessageBus）方案

> 状态：已完成（2026-07-03）。本文档作为架构说明保留。

## 1. 背景与问题

跨实例 WebSocket 广播依赖 Redis Pub/Sub。现状是每个广播器各自调用
`RedisClient::subscribe(channel, cb)`，每个订阅持有**一条独立 Redis 连接 + 一个独立后台线程**
（`RedisSubscription`），且五个广播器的代码模式完全相同：

```
start()  → subscription_ = RedisClient.subscribe(自己的频道, 回调)
notify() → if (subscription_ && publish(频道)) return; 否则本地直触
stop()   → subscription_->stop()
```

现有 5 个频道：`realtime:admin-home` / `realtime:finance-home` / `realtime:doctor-queue` /
`realtime:medicine-stock` / `realtime:doctor-list`。**频道每 +1 → 连接 +1、线程 +1、样板 +1 份**，
随业务增长线性膨胀。

## 2. 目标设计

新增 `services/redis/redisMessageBus/RedisMessageBus.{h,cpp}`（遵循项目"Redis 按域模块化"约定），
单例，全进程**一条订阅连接 + 一个分发线程**：

```cpp
RedisMessageBus::instance()
  .subscribe(channel, handler);  // 注册频道→回调；首次调用惰性启动总线线程
  .active();                     // 总线是否在跑（替代原 subscription_ 非空判断）
  .stop();                       // 优雅关停（加入 setRoutes 关停序列）
```

### 内部机制

1. **单连接单线程**：一次 `SUBSCRIBE ch1 ch2 ...`（`redisCommandArgv` 变长参数）订阅全部已注册频道；
   收到 `["message", channel, payload]` 后按 channel 查表分发给对应 handler。
2. **动态加频道**：总线运行后再注册新频道 → 置 `resubscribe` 标志 + `shutdown(fd)` 打断阻塞读 →
   循环顶部重连并按**当前完整频道集**重新 SUBSCRIBE（复用原 `RedisSubscription::stop` 的打断手法）。
3. **连接知识收归 RedisClient**：新增 `RedisClient::createSubscriberConnection()`
   （connect + AUTH + SELECT，返回独立阻塞连接）；总线只管分发，不碰连接参数。
4. **降级语义不变**：Redis 未启用 → `active()` 为 false，广播器 `notify` 走原有本地直触回退；
   handler 异常被总线吞掉，不拖垮分发线程。
5. **handler 约定**：在总线线程**串行**执行，必须轻量（置 pending 标志 / notify 条件变量级别）；
   重活留在各广播器自己的广播线程（现有模式本就如此）。

## 3. 改动清单

| # | 内容 |
|---|---|
| 1 | 新增 `services/redis/redisMessageBus/RedisMessageBus.{h,cpp}`；`RedisClient` 增加 `createSubscriberConnection()` |
| 2 | 5 个广播器迁移：删 `subscription_` 成员；`start()` 改注册到总线；`notify()` 的 `subscription_ &&` 改 `RedisMessageBus::instance().active() &&`；`stop()` 删退订块。频道名常量仍留各自模块（"key 跟模块走"约定） |
| 3 | `setRoutes.cpp` 优雅关停序列末尾加 `RedisMessageBus::instance().stop()` |
| 4 | 删除 `RedisSubscription` 类与 `RedisClient::subscribe()`（5 个广播器是仅有消费者；双轨并存会诱导新代码继续走旧路） |
| 5 | `bin/build.sh` 验证 + 结构自验；更新项目 MEMORY.md 中的广播架构记录 |

## 4. 收益 / 代价 / 风险

- **收益**：5 连接 5 线程 → 1 连接 1 线程；以后新增频道 = 一行
  `RedisMessageBus::instance().subscribe(channel, cb)`，零新线程、零新连接、零样板。
- **代价**：所有频道的回调在同一线程串行分发。现有 5 个回调均为微秒级唤醒操作，无阻塞风险；
  新 handler 必须遵守轻量约定（见上 §2.5）。
- **风险**：动态重订阅的重连间隙（毫秒级）可能丢消息。对本项目无影响：所有消息都是"刷新信号"，
  丢失至多延迟到下次变更；且频道实际都在启动期注册完毕，运行期重连只发生在断线（与现状一致）。

## 5. 新增频道指南（给未来的自己）

```cpp
// 1. 频道名常量放在你的业务模块里（不要集中注册表）
constexpr const char *kMyChannel = "realtime:my-domain";

// 2. 启动时注册（回调必须轻量：置标志/唤醒条件变量，别做 IO）
RedisMessageBus::instance().subscribe(kMyChannel,
    [this](const std::string &payload) { triggerLocalXxx(); });

// 3. 变更时发布（active() 判断 Redis 路径可用，失败回退本地直触）
if (RedisMessageBus::instance().active() &&
    RedisClient::instance().publish(kMyChannel, "1")) return;
triggerLocalXxx();
```
