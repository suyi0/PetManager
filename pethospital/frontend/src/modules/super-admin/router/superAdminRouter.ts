import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";
import { SUPER_ADMIN_PORTAL_ROLES } from "@/core/auth/utils/roleUtils";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/super-admin",
    component: () => import("../views/SuperAdminLayout.vue"),
    meta: { requiresAuth: true, allowedRoles: [...SUPER_ADMIN_PORTAL_ROLES] },
    children: [
      {
        path: "",
        redirect: "/super-admin/overview",
      },
      {
        path: "overview",
        name: "superAdminOverview",
        component: () => import("../views/pages/SuperAdminOverview.vue"),
      },
      {
        path: "worktime",
        name: "superAdminWorktime",
        component: () => import("../views/pages/SuperAdminWorktime.vue"),
      },
      {
        path: "users",
        name: "superAdminUsers",
        component: () => import("../views/pages/SuperAdminUsers.vue"),
      },
      {
        path: "online-doctors",
        name: "superAdminOnlineDoctors",
        component: () => import("../views/pages/SuperAdminOnlineDoctors.vue"),
      },
      {
        path: "users/:userId",
        name: "superAdminUserDetail",
        component: () => import("../views/pages/SuperAdminUserDetail.vue"),
      },
      {
        path: "logs",
        name: "superAdminLogs",
        component: () => import("../views/pages/SuperAdminLogs.vue"),
      },
      {
        path: "personal",
        name: "superAdminPersonal",
        component: () =>
          import(
            "@/modules/user/views/desktop/Personal-module/UserPersonalView.vue"
          ),
        props: { roleBadge: "管理员端" },
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
