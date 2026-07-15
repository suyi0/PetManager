#pragma once

#include "../../database/DatabaseManagerInterface.h"

#include <memory>

// 任职 outbox 幂等消费：会话撤销、设备同步、医生缓存/广播。
// 以 employment_event_outbox 行为权威；重复消费必须安全。
namespace EmploymentOutboxDispatcher
{
// 消费指定 outbox id（提交后调用）；已 completed 的事件直接成功返回。
bool dispatchOne(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    long long outboxId);

// 扫描并消费 pending 事件（启动恢复 / 重试）。
int dispatchPending(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int limit = 50);
}
