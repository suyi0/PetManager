export type UserRole = "超级管理员" | "医生" | "仓库管理员" | "普通用户";

export const resolveRoleName = (
  typeName?: string | null,
  typeId?: number | null
): UserRole | null => {
  if (typeName === "超级管理员") return "超级管理员";
  if (typeName === "医生") return "医生";
  if (typeName === "仓库管理员") return "仓库管理员";
  if (typeName === "普通用户") return "普通用户";

  if (typeId === 1) return "超级管理员";
  if (typeId === 2) return "医生";
  if (typeId === 3) return "仓库管理员";
  if (typeId === 4) return "普通用户";

  return null;
};

export const getHomeRouteByRole = (role?: string | null) => {
  switch (resolveRoleName(role)) {
    case "超级管理员":
      return "/super-admin/overview";
    case "医生":
      return "/doctor/home";
    case "仓库管理员":
      return "/warehouse-admin/dashboard";
    case "普通用户":
      return "/user/home";
    default:
      return "/";
  }
};
