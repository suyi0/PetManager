import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/super-admin",
    component: () => import("../views/SuperAdminLayout.vue"),
    meta: { requiresAuth: true, allowedRoles: ["超级管理员"] },
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
        path: "doctors",
        name: "superAdminRoleAccess",
        component: () => import("../views/pages/SuperAdminRoleAccess.vue"),
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
        path: "logs",
        name: "superAdminLogs",
        component: () => import("../views/pages/SuperAdminLogs.vue"),
      },
    ],
  },
  {
    path: "/preview/super-admin",
    component: () => import("../views/SuperAdminLayout.vue"),
    children: [
      {
        path: "",
        redirect: "/preview/super-admin/overview",
      },
      {
        path: "overview",
        name: "previewSuperAdminOverview",
        component: () => import("../views/pages/SuperAdminOverview.vue"),
      },
      {
        path: "doctors",
        name: "previewSuperAdminRoleAccess",
        component: () => import("../views/pages/SuperAdminRoleAccess.vue"),
      },
      {
        path: "worktime",
        name: "previewSuperAdminWorktime",
        component: () => import("../views/pages/SuperAdminWorktime.vue"),
      },
      {
        path: "users",
        name: "previewSuperAdminUsers",
        component: () => import("../views/pages/SuperAdminUsers.vue"),
      },
      {
        path: "logs",
        name: "previewSuperAdminLogs",
        component: () => import("../views/pages/SuperAdminLogs.vue"),
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
