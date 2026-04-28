import { RouteRecordRaw } from "vue-router";
import { USER_PORTAL_ROLES } from "@/core/auth/utils/roleUtils";

const UserRoutes: Array<RouteRecordRaw> = [
  {
    path: "/user",
    component: () => import("../views/UserHome-module/UserLayout.vue"),
    meta: { requiresAuth: true, allowedRoles: [...USER_PORTAL_ROLES] },
    children: [
      {
        path: "",
        redirect: "/user/home",
      },
      {
        path: "home",
        name: "home",
        component: () =>
          import("../views/UserHome-module/HomeNav/UserHome.vue"),
      },
      {
        path: "services",
        name: "userServices",
        component: () =>
          import("../views/UserHome-module/HomeNav/UserServices.vue"),
      },
      {
        path: "/user/personal",
        name: "userPersonal",
        component: () =>
          import("../views/Personal-module/UserPersonalView.vue"),
        meta: { requiresAuth: true, allowedRoles: [...USER_PORTAL_ROLES] }, // 需要认证才能访问
      },
      {
        path: "/user/order",
        component: () =>
          import("../views/UserHome-module/HomeNav/order-module/orderView.vue"),
        name: "userOrder",
        meta: { requiresAuth: true, allowedRoles: [...USER_PORTAL_ROLES] }, // 需要认证才能访问
      },
      {
        path: "/user/orderDetail",
        component: () =>
          import(
            "../views/UserHome-module/HomeNav/order-module/orderDetail.vue"
          ),
        name: "userOrderDetail",
        meta: { requiresAuth: true, allowedRoles: [...USER_PORTAL_ROLES] }, // 需要认证才能访问
      },
    ],
  },
  {
    path: "/preview/user",
    component: () => import("../views/UserHome-module/UserLayout.vue"),
    children: [
      {
        path: "",
        redirect: "/preview/user/home",
      },
      {
        path: "home",
        name: "previewUserHome",
        component: () =>
          import("../views/UserHome-module/HomeNav/UserHome.vue"),
      },
      {
        path: "services",
        name: "previewUserServices",
        component: () =>
          import("../views/UserHome-module/HomeNav/UserServices.vue"),
      },
      {
        path: "personal",
        name: "previewUserPersonal",
        component: () =>
          import("../views/Personal-module/UserPersonalView.vue"),
      },
      {
        path: "order",
        name: "previewUserOrder",
        component: () =>
          import("../views/UserHome-module/HomeNav/order-module/orderView.vue"),
      },
      {
        path: "orderDetail",
        name: "previewUserOrderDetail",
        component: () =>
          import(
            "../views/UserHome-module/HomeNav/order-module/orderDetail.vue"
          ),
      },
    ],
  },
];

export default UserRoutes;
