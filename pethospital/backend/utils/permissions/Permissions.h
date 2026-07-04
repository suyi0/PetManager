#pragma once

#include <string>

namespace Permissions
{
inline constexpr const char *kPortalBoss = "portal:boss";
inline constexpr const char *kPortalFinance = "portal:finance";
inline constexpr const char *kPortalSuperAdmin = "portal:super-admin";
inline constexpr const char *kPortalPersonnel = "portal:personnel";
inline constexpr const char *kPortalMedical = "portal:medical";
inline constexpr const char *kPortalWarehouse = "portal:warehouse";

inline constexpr const char *kSalaryRead = "salary:read";
inline constexpr const char *kSalaryWrite = "salary:write";
inline constexpr const char *kLogsRead = "logs:read";
inline constexpr const char *kUserDelete = "user:delete";
inline constexpr const char *kEquityRead = "equity:read";
inline constexpr const char *kEquityWrite = "equity:write";
inline constexpr const char *kStockRead = "stock:read";
inline constexpr const char *kStockWrite = "stock:write";
inline constexpr const char *kScopeAll = "scope:all";
inline constexpr const char *kScopeMedicalAssigned = "scope:medical-assigned";

bool roleHasPermission(const std::string &roleName, const std::string &permissionKey);
}
