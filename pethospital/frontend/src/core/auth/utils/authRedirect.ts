import { getHomeRouteByRole, resolveRoleName } from "./roleUtils";

export function getHomeRouteByUserType(
  userType: number | null | undefined,
  userRole?: string | null
) {
  return getHomeRouteByRole(resolveRoleName(userRole, userType));
}
