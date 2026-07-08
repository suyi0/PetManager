// 门户访问的前端判据 —— 唯一数据源是后端下发的权限集(permissions)与
// account_type / staff_kind(登录响应 access 字段或 /api/auth/me)。
// 职位显示名(type_name/userRole)只做展示,禁止用于任何判权/导航/存储决策
// (动态角色下名字可改可新增,按名字判断=双轨漏洞,见 RBAC-DYNAMIC-ROLES-DESIGN.md §6)。
// 权限 key 与后端 utils/permissions/Permissions.h 保持一致。

export const PORTAL_PERMISSIONS = {
  boss: "portal:boss",
  finance: "portal:finance",
  superAdmin: "portal:super-admin",
  personnel: "portal:personnel",
  medical: "portal:medical",
  warehouse: "portal:warehouse",
  user: "portal:user",
} as const;

export interface AccessSnapshot {
  permissions?: string[] | null;
  accountType?: string | null;
  staffKind?: string | null;
}

export const hasPermission = (
  access: AccessSnapshot | null | undefined,
  permissionKey: string
): boolean => Boolean(access?.permissions?.includes(permissionKey));

// 管理端会话(sessionStorage 隔离、随标签页销毁)的判据:
// 等价旧 SUPER_ADMIN_PORTAL_ROLES(总裁/副总裁/部门经理/超级管理员)= portal:super-admin 持有者
export const isManagementPortalSession = (
  access: AccessSnapshot | null | undefined
): boolean => hasPermission(access, PORTAL_PERMISSIONS.superAdmin);

// 登录后首页 —— 判定顺序与旧 getHomeRouteByRole 等价:
// boss → super-admin → doctor(工种) → finance → personnel → warehouse → user(客户/护士/portal:user)
export const getHomeRouteByAccess = (
  access: AccessSnapshot | null | undefined
): string => {
  if (!access) {
    return "/";
  }
  if (hasPermission(access, PORTAL_PERMISSIONS.boss)) {
    return "/boss/overview";
  }
  if (hasPermission(access, PORTAL_PERMISSIONS.superAdmin)) {
    return "/super-admin/overview";
  }
  // 医生进诊疗工作台;护士虽同持 portal:medical,旧行为是进用户端首页 —— 用工种区分
  if (access.staffKind === "doctor" && hasPermission(access, PORTAL_PERMISSIONS.medical)) {
    return "/doctor/home";
  }
  if (hasPermission(access, PORTAL_PERMISSIONS.finance)) {
    return "/finance/salary";
  }
  if (hasPermission(access, PORTAL_PERMISSIONS.personnel)) {
    return "/personnel/access";
  }
  if (hasPermission(access, PORTAL_PERMISSIONS.warehouse)) {
    return "/warehouse-admin/dashboard";
  }
  if (
    access.accountType === "customer" ||
    hasPermission(access, PORTAL_PERMISSIONS.user) ||
    hasPermission(access, PORTAL_PERMISSIONS.medical) // 护士等医护:用户端首页(等价旧 USER_PORTAL_ROLES 含护士)
  ) {
    return "/user/home";
  }
  return "/";
};
