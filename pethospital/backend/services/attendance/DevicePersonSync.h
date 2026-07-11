#pragma once

#include "../../database/DatabaseManagerInterface.h"

#include <memory>
#include <string>

// 人员模板与考勤设备的下发队列（v1 只落任务标记，P3 由 vendor adapter 消费）。
// 语义按 desired_state 建模：同一 (device_id, user_id) 一行，重复 enqueue 幂等覆盖，
// 消费者只需把设备端状态推平到 desired_state。
// 队列是辅助链路：失败只记日志，绝不阻断人事主流程。
namespace DevicePersonSync
{
// 员工缺考勤号时按设计初始化为 users.id 字符串；返回最终考勤号（非员工/已删除返回空）。
std::string ensureAttendanceNo(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId);

// 入职/启用：对所有 active 设备生成/重置 upsert 任务（内部先 ensureAttendanceNo）。
void enqueueUpsert(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId);

// 离职/转客户：把该用户在所有设备上的任务标记为待删除（AccessRevocation 之外的第四件套）。
void enqueueRemove(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId);

// 新设备注册：把当前所有在职且有考勤号的员工补发到该设备。
void enqueueDeviceBackfill(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int deviceId);
}
