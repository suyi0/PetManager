export function getHomeRouteByUserType(userType: number | null | undefined) {
  switch (userType) {
    case 1:
      return "/super-admin/overview";
    case 2:
      return "/doctor/home";
    case 3:
      return "/user/home";
    default:
      return "/";
  }
}
