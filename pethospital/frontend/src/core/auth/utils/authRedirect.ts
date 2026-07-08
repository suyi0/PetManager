import {
  AccessSnapshot,
  getHomeRouteByAccess,
} from "./portalAccess";

// 登录后/已登录访客的首页解析。
// 数据源是权限集(access.permissions)+account_type/staff_kind,不再按职位名映射——
// 动态新职位按其持有的门户权限落到对应首页,而不是落回 "/"。
export function getHomeRouteByUserAccess(
  access: AccessSnapshot | null | undefined
) {
  return getHomeRouteByAccess(access);
}
