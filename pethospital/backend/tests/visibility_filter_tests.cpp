#include "../utils/visibilityFilter/VisibilityFilter.h"

#include <cassert>
#include <string>

// 逐字节锁定 orderCommon / reservationCommon / searchCommon 的可见性过滤 SQL 片段。
// 任何一个字符（含结尾空格）漂移都会挂测试——这是可见性过滤的安全契约。
int main()
{
    using VisibilityFilter::build;
    using VisibilityFilter::Clause;

    // ---- 全部四个生产调用点现在都用 alwaysExcludeSoftDeleted=true：
    //      所有角色（含 Boss / 医护）都排除软删行，列表 summary 与关键字 search 行为一致。----

    // orderCommon::getOrderSummary + searchCommon orders（alias=o, owner=owner_id）
    {
        Clause boss = build(true, false, "o", "owner_id", true);
        assert(boss.whereSql == "WHERE o.is_deleted = 0 ");
        assert(!boss.bindsUserId);

        Clause medical = build(false, true, "o", "owner_id", true);
        assert(medical.whereSql == "WHERE o.doctor_id = ? AND o.is_deleted = 0 ");
        assert(medical.bindsUserId);

        Clause owner = build(false, false, "o", "owner_id", true);
        assert(owner.whereSql == "WHERE o.owner_id = ? AND o.is_deleted = 0 ");
        assert(owner.bindsUserId);
    }

    // reservationCommon::getReservationSummary + searchCommon reservations（alias=r, owner=user_id）
    {
        Clause boss = build(true, false, "r", "user_id", true);
        assert(boss.whereSql == "WHERE r.is_deleted = 0 ");
        assert(!boss.bindsUserId);

        Clause medical = build(false, true, "r", "user_id", true);
        assert(medical.whereSql == "WHERE r.doctor_id = ? AND r.is_deleted = 0 ");
        assert(medical.bindsUserId);

        Clause owner = build(false, false, "r", "user_id", true);
        assert(owner.whereSql == "WHERE r.user_id = ? AND r.is_deleted = 0 ");
        assert(owner.bindsUserId);
    }

    // ---- 函数 false 模式契约（当前无生产调用点，保留以覆盖分支）：
    //      Boss 无 WHERE、医护不追加 is_deleted 过滤。若未来有"含软删历史"视图才会用到。----
    {
        Clause boss = build(true, false, "o", "owner_id", false);
        assert(boss.whereSql == "");
        assert(!boss.bindsUserId);

        Clause medical = build(false, true, "o", "owner_id", false);
        assert(medical.whereSql == "WHERE o.doctor_id = ? ");
        assert(medical.bindsUserId);

        // 普通用户始终排除软删，false 模式下与 true 模式一致。
        Clause owner = build(false, false, "r", "user_id", false);
        assert(owner.whereSql == "WHERE r.user_id = ? AND r.is_deleted = 0 ");
        assert(owner.bindsUserId);
    }

    // ---- fail-closed：既非 Boss 也非医护（如空角色名 / 未知角色）落到最窄的 owner 作用域 ----
    {
        Clause fallback = build(false, false, "o", "owner_id", true);
        assert(fallback.whereSql == "WHERE o.owner_id = ? AND o.is_deleted = 0 ");
        assert(fallback.bindsUserId); // 必须绑定 userId，绝不放行为全量可见
    }

    return 0;
}
