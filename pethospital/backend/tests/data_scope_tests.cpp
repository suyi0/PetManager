#include "../utils/dataScope/DataScope.h"
#include "../utils/permissions/Permissions.h"
#include "../utils/visibilityFilter/VisibilityFilter.h"

#include <cassert>

int main()
{
    {
        DataScope::Scope boss = DataScope::resolveForRole("总裁", 42);
        assert(boss.kind == DataScope::Kind::All);
        assert(boss.userId == 42);
        assert(!DataScope::bindsUserId(boss));
        assert(Permissions::roleHasPermission("总裁", Permissions::kScopeAll));
    }

    {
        DataScope::Scope medical = DataScope::resolveForRole("医生", 42);
        assert(medical.kind == DataScope::Kind::MedicalAssigned);
        assert(medical.userId == 42);
        assert(DataScope::bindsUserId(medical));
        assert(Permissions::roleHasPermission("医生", Permissions::kScopeMedicalAssigned));
    }

    {
        DataScope::Scope nurse = DataScope::resolveForRole("护士", 7);
        assert(nurse.kind == DataScope::Kind::MedicalAssigned);
        assert(nurse.userId == 7);
        assert(DataScope::bindsUserId(nurse));
    }

    {
        DataScope::Scope owner = DataScope::resolveForRole("普通用户", 42);
        assert(owner.kind == DataScope::Kind::Owner);
        assert(owner.userId == 42);
        assert(DataScope::bindsUserId(owner));
        assert(!Permissions::roleHasPermission("普通用户", Permissions::kScopeAll));
        assert(!Permissions::roleHasPermission("普通用户", Permissions::kScopeMedicalAssigned));
    }

    {
        DataScope::Scope fallback = DataScope::resolveForRole("医生助理", 42);
        assert(fallback.kind == DataScope::Kind::Owner);
        assert(fallback.userId == 42);
        assert(DataScope::bindsUserId(fallback));
    }

    {
        DataScope::Scope empty = DataScope::resolveForRole("", 42);
        assert(empty.kind == DataScope::Kind::Owner);
        assert(empty.userId == 42);
        assert(DataScope::bindsUserId(empty));
    }

    {
        VisibilityFilter::Clause boss =
            VisibilityFilter::build(DataScope::resolveForRole("总裁", 42), "o", "owner_id", true);
        assert(boss.whereSql == "WHERE o.is_deleted = 0 ");
        assert(!boss.bindsUserId);

        VisibilityFilter::Clause medical =
            VisibilityFilter::build(DataScope::resolveForRole("医生", 42), "o", "owner_id", true);
        assert(medical.whereSql == "WHERE o.doctor_id = ? AND o.is_deleted = 0 ");
        assert(medical.bindsUserId);

        VisibilityFilter::Clause owner =
            VisibilityFilter::build(DataScope::resolveForRole("普通用户", 42), "o", "owner_id", true);
        assert(owner.whereSql == "WHERE o.owner_id = ? AND o.is_deleted = 0 ");
        assert(owner.bindsUserId);
    }

    return 0;
}
