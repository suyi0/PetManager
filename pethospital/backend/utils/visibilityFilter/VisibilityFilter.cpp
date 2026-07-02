#include "VisibilityFilter.h"

namespace VisibilityFilter
{
Clause build(
    bool isBoss,
    bool isMedicalStaff,
    const std::string &alias,
    const std::string &ownerColumn,
    bool alwaysExcludeSoftDeleted)
{
    const std::string softDeleted = "AND " + alias + ".is_deleted = 0 ";

    if (isBoss)
    {
        // Boss 看全部；search 场景需要一个先导 WHERE 承接后续的 AND(关键字)。
        return {alwaysExcludeSoftDeleted ? ("WHERE " + alias + ".is_deleted = 0 ") : "", false};
    }

    if (isMedicalStaff)
    {
        std::string clause = "WHERE " + alias + ".doctor_id = ? ";
        if (alwaysExcludeSoftDeleted)
        {
            clause += softDeleted;
        }
        return {clause, true};
    }

    // 普通用户等：始终排除软删。
    return {"WHERE " + alias + "." + ownerColumn + " = ? " + softDeleted, true};
}
}
