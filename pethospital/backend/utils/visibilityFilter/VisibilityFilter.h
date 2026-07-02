#pragma once

#include <string>

// 角色可见性过滤：orderCommon / reservationCommon / searchCommon 三处
// 共享同一套「谁能看到哪些行」的规则，过去各自手写三元表达式，容易漂移。
// 这里把它收敛成一个无依赖的纯函数，便于逐字节锁定 SQL 片段并单测。
//
// 可见性契约（与既有 handler 行为逐字一致，本次仅抽取不改语义）：
//   - Boss（总裁/副总裁）：看全部行；
//   - 医护（医生/护士）：仅看 doctor_id = 自己 的行；
//   - 其余（普通用户等）：仅看 ownerColumn = 自己 的行，且始终排除软删。
//
// alwaysExcludeSoftDeleted 表达 summary 与 search 的唯一差异：
//   - false（列表 summary）：Boss 无 WHERE、医护不加 is_deleted 过滤；
//   - true （关键字 search）：Boss / 医护也追加 is_deleted = 0，
//     因为其后紧跟 "AND (关键字...)"，需要一个先导 WHERE 让 AND 合法。
namespace VisibilityFilter
{
struct Clause
{
    // WHERE/过滤片段（含结尾空格；Boss + summary 时为空串）。
    std::string whereSql;
    // 是否需要把 userId 绑定进查询（Boss 为 false，其余为 true）。
    bool bindsUserId;
};

// isBoss / isMedicalStaff 由调用方经 RoleTypeUtils 判定后传入。
// alias：表别名（如 "o" / "r"）；ownerColumn：普通用户归属列（如 "owner_id" / "user_id"）。
Clause build(
    bool isBoss,
    bool isMedicalStaff,
    const std::string &alias,
    const std::string &ownerColumn,
    bool alwaysExcludeSoftDeleted);
}
