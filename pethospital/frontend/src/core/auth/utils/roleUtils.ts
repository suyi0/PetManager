export const ALL_ROLE_NAMES = [
  "总裁",
  "副总裁",
  "财务总监",
  "财务经理",
  "人事经理",
  "部门经理",
  "超级管理员",
  "仓库管理员",
  "医生",
  "护士",
  "普通用户",
] as const;

export type UserRole = (typeof ALL_ROLE_NAMES)[number];

export const SUPER_ADMIN_PORTAL_ROLES: UserRole[] = [
  "总裁",
  "副总裁",
  "部门经理",
  "超级管理员",
];

export const BOSS_PORTAL_ROLES: UserRole[] = ["总裁", "副总裁"];

export const FINANCE_PORTAL_ROLES: string[] = [
  ...BOSS_PORTAL_ROLES,
  "财务总监",
  "财务经理",
];

export const PERSONNEL_PORTAL_ROLES: string[] = [
  ...BOSS_PORTAL_ROLES,
  "人事经理",
];

export const WAREHOUSE_PORTAL_ROLES: UserRole[] = [
  ...BOSS_PORTAL_ROLES,
  "仓库管理员",
];

export const DOCTOR_PORTAL_ROLES: UserRole[] = [...BOSS_PORTAL_ROLES, "医生"];

export const USER_PORTAL_ROLES: UserRole[] = [
  ...BOSS_PORTAL_ROLES,
  "普通用户",
  "护士",
];

const ROLE_NAME_SET = new Set<string>(ALL_ROLE_NAMES);

export const isKnownRoleName = (role?: string | null): role is UserRole =>
  Boolean(role && ROLE_NAME_SET.has(role));

export const isSuperAdminPortalRole = (role?: string | null): boolean =>
  Boolean(role && SUPER_ADMIN_PORTAL_ROLES.includes(role as UserRole));

export const isBossPortalRole = (role?: string | null): boolean =>
  Boolean(role && BOSS_PORTAL_ROLES.includes(role as UserRole));

export const isWarehousePortalRole = (role?: string | null): boolean =>
  Boolean(role && WAREHOUSE_PORTAL_ROLES.includes(role as UserRole));

export const isFinancePortalRole = (role?: string | null): boolean =>
  Boolean(role && FINANCE_PORTAL_ROLES.includes(role));

export const isDoctorPortalRole = (role?: string | null): boolean =>
  Boolean(role && DOCTOR_PORTAL_ROLES.includes(role as UserRole));

export const isPersonnelPortalRole = (role?: string | null): boolean =>
  Boolean(role && PERSONNEL_PORTAL_ROLES.includes(role));

export const isUserPortalRole = (role?: string | null): boolean =>
  Boolean(role && USER_PORTAL_ROLES.includes(role as UserRole));

export const resolveRoleName = (
  typeName?: string | null,
  typeId?: number | null
): UserRole | null => {
  if (isKnownRoleName(typeName)) {
    return typeName;
  }

  void typeId;

  return null;
};

export const getHomeRouteByRole = (role?: string | null) => {
  const resolvedRole = resolveRoleName(role);

  if (isBossPortalRole(resolvedRole)) {
    return "/boss/overview";
  }

  if (isSuperAdminPortalRole(resolvedRole)) {
    return "/super-admin/overview";
  }

  if (isDoctorPortalRole(resolvedRole)) {
    return "/doctor/home";
  }

  if (isFinancePortalRole(resolvedRole)) {
    return "/finance/salary";
  }

  if (isPersonnelPortalRole(resolvedRole)) {
    return "/personnel/access";
  }

  if (isWarehousePortalRole(resolvedRole)) {
    return "/warehouse-admin/dashboard";
  }

  if (isUserPortalRole(resolvedRole)) {
    return "/user/home";
  }

  return "/";
};
