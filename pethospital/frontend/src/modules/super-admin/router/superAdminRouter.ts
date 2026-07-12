import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/super-admin",
    component: () => import("../views/SuperAdminLayout.vue"),
    meta: {
      requiresAuth: true,
      allowedPermissions: ["portal:super-admin"],
    },
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
        path: "rbac",
        name: "superAdminRbac",
        component: () => import("../views/pages/SuperAdminRbac.vue"),
      },
      {
        path: "rbac/positions",
        name: "superAdminRbacPositions",
        component: () => import("../views/pages/SuperAdminRbacPositions.vue"),
      },
      {
        path: "rbac/users",
        name: "superAdminRbacUsers",
        component: () => import("../views/pages/SuperAdminRbacUsers.vue"),
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
        path: "report-templates",
        name: "superAdminReportTemplates",
        component: () => import("../views/pages/SuperAdminReportTemplates.vue"),
        meta: { allowedPermissions: ["report-template:read"] },
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
