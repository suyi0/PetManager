// 纯展示工具 —— 职位名的徽标配色 / 历史日志筛选清单 / 显示兜底。
// 禁止用于判权、导航、存储决策:那些一律走 store.auth.permissions(见 portalAccess.ts)。
// 动态角色下职位名可增可改,按名字只允许影响"长什么样",不允许影响"能不能"。

// 历史操作日志里 system_role/user_role 存的是当时职位名快照;
// 这份清单只覆盖内置 seed 职位,用于日志筛选下拉的快捷项。
// 动态新增职位的日志仍可显示,只是不在快捷清单里(后续可改为从 positions API 拉取)。
export const LEGACY_ROLE_NAMES = [
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

// 管理系徽标配色(等价旧 SUPER_ADMIN_PORTAL_ROLES);未知/动态职位落默认色
export const MANAGEMENT_PILL_ROLE_NAMES = new Set<string>([
  "总裁",
  "副总裁",
  "部门经理",
  "超级管理员",
]);

export const isManagementPillRole = (role?: string | null): boolean =>
  Boolean(role && MANAGEMENT_PILL_ROLE_NAMES.has(role));

// 展示名:动态职位名原样显示(不再按旧枚举过滤成"未知角色")
export const displayRoleName = (
  name?: string | null,
  fallback = "未知角色"
): string => name || fallback;
