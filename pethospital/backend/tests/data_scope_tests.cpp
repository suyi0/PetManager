#include "../utils/dataScope/DataScope.h"
#include "../utils/visibilityFilter/VisibilityFilter.h"

#include <cassert>
#include <memory>

// 数据范围契约（动态 RBAC 版）：
// 1. resolveForUser 是唯一入口——按用户当前职位的 scope:* 权限查库解析，
//    职位名不参与判定（名字可被超管改，判定必须库驱动）。
// 2. fail-closed：无数据库连接 / 用户不存在 / 客户账户（无职位）一律落到
//    Owner（只看自己），绝不放大可见范围。
// 3. VisibilityFilter 的 SQL 生成契约保持不变（All 免 owner 过滤 /
//    MedicalAssigned 绑 doctor_id / Owner 绑 owner_id）。
// 角色→scope 权限的映射正确性由 seed 等价性测试（rbac_schema_tests）锁定。
int main()
{
    const std::shared_ptr<DatabaseManagerInterface> nullDb;

    // ---- fail-closed：无数据库连接 → Owner ----
    {
        DataScope::Scope scope = DataScope::resolveForUser(nullDb, 42);
        assert(scope.kind == DataScope::Kind::Owner);
        assert(scope.userId == 42);
        assert(DataScope::bindsUserId(scope));
    }

    // ---- fail-closed：非法 userId → Owner ----
    {
        DataScope::Scope scope = DataScope::resolveForUser(nullDb, 0);
        assert(scope.kind == DataScope::Kind::Owner);
        DataScope::Scope negative = DataScope::resolveForUser(nullDb, -1);
        assert(negative.kind == DataScope::Kind::Owner);
    }

    // ---- bindsUserId 语义：只有 All 免绑定 ----
    {
        assert(!DataScope::bindsUserId({DataScope::Kind::All, 42}));
        assert(DataScope::bindsUserId({DataScope::Kind::MedicalAssigned, 42}));
        assert(DataScope::bindsUserId({DataScope::Kind::Owner, 42}));
    }

    // ---- VisibilityFilter SQL 生成契约 ----
    {
        VisibilityFilter::Clause all =
            VisibilityFilter::build({DataScope::Kind::All, 42}, "o", "owner_id", true);
        assert(all.whereSql == "WHERE o.is_deleted = 0 ");
        assert(!all.bindsUserId);

        VisibilityFilter::Clause medical =
            VisibilityFilter::build({DataScope::Kind::MedicalAssigned, 42}, "o", "owner_id", true);
        assert(medical.whereSql == "WHERE o.doctor_id = ? AND o.is_deleted = 0 ");
        assert(medical.bindsUserId);

        VisibilityFilter::Clause owner =
            VisibilityFilter::build({DataScope::Kind::Owner, 42}, "o", "owner_id", true);
        assert(owner.whereSql == "WHERE o.owner_id = ? AND o.is_deleted = 0 ");
        assert(owner.bindsUserId);
    }

    return 0;
}
