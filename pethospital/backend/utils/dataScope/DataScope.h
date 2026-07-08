#pragma once

#include <memory>

// 前置声明即可：shared_ptr<不完整类型> 作函数参数合法。
// 不 include DatabaseManagerInterface.h，避免把 mysqlx 头传染给所有 DataScope 使用者
// （VisibilityFilter 及其测试无需链接 mysqlx）。完整定义只在 DataScope.cpp 引入。
class DatabaseManagerInterface;

namespace DataScope
{
enum class Kind
{
    All,
    MedicalAssigned,
    Owner
};

struct Scope
{
    Kind kind;
    int userId;
};

// 按用户当前职位权限解析数据范围（scope:all / scope:medical-assigned / 默认只看自己）。
// 判据是 position_permissions（RbacService），不是职位名——职位名只做展示。
Scope resolveForUser(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId);

bool bindsUserId(const Scope &scope);
}
