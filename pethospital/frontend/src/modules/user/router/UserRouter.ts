import { RouteRecordRaw } from "vue-router";

const UserRoutes: Array<RouteRecordRaw> = [
  {
    path: "/user",
    component: () => import("../views/desktop/UserHome-module/UserLayout.vue"),
    meta: {
      requiresAuth: true,
      allowedPermissions: ["portal:user", "portal:boss"],
      allowedStaffKinds: ["nurse"],
    },
    children: [
      {
        path: "",
        redirect: "/user/home",
      },
      {
        path: "home",
        name: "home",
        component: () =>
          import("../views/desktop/UserHome-module/HomeNav/UserHome.vue"),
      },
      {
        path: "services",
        name: "userServices",
        component: () =>
          import(
            "../views/desktop/UserHome-module/HomeNav/Services-module/UserServices.vue"
          ),
      },
      {
        path: "/user/personal",
        name: "userPersonal",
        component: () =>
          import("../views/desktop/Personal-module/UserPersonalView.vue"),
        meta: {
          requiresAuth: true,
          allowedPermissions: ["portal:user", "portal:boss"],
          allowedStaffKinds: ["nurse"],
        }, // 需要认证才能访问
      },
      {
        path: "/user/order",
        component: () =>
          import(
            "../views/desktop/UserHome-module/HomeNav/order-module/orderView.vue"
          ),
        name: "userOrder",
        meta: {
          requiresAuth: true,
          allowedPermissions: ["portal:user", "portal:boss"],
          allowedStaffKinds: ["nurse"],
        }, // 需要认证才能访问
      },
      {
        path: "/user/attendance",
        name: "userAttendance",
        component: () =>
          import("../views/desktop/Personal-module/UserAttendanceView.vue"),
        meta: {
          requiresAuth: true,
          allowedPermissions: ["portal:user", "portal:boss"],
          allowedStaffKinds: ["nurse"],
        },
      },
      {
        path: "/user/medical-documents",
        name: "userMedicalDocuments",
        component: () =>
          import("../views/desktop/Personal-module/UserMedicalDocuments.vue"),
      },
      {
        path: "/user/orderDetail",
        component: () =>
          import(
            "../views/desktop/UserHome-module/HomeNav/order-module/orderDetail.vue"
          ),
        name: "userOrderDetail",
        meta: {
          requiresAuth: true,
          allowedPermissions: ["portal:user", "portal:boss"],
          allowedStaffKinds: ["nurse"],
        }, // 需要认证才能访问
      },
    ],
  },
];

export default UserRoutes;
