import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";
import { WAREHOUSE_PORTAL_ROLES } from "@/core/auth/utils/roleUtils";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/warehouse-admin",
    component: () => import("../views/WarehouseAdminLayout.vue"),
    meta: { requiresAuth: true, allowedRoles: [...WAREHOUSE_PORTAL_ROLES] },
    children: [
      {
        path: "",
        redirect: "/warehouse-admin/dashboard",
      },
      {
        path: "overview",
        redirect: "/warehouse-admin/dashboard",
      },
      {
        path: "dashboard",
        name: "warehouseAdminDashboard",
        component: () => import("../views/pages/WarehouseAdminDashboard.vue"),
      },
      {
        path: "create",
        name: "warehouseAdminCreate",
        component: () => import("../views/pages/WarehouseAdminCreate.vue"),
      },
      {
        path: "warnings",
        name: "warehouseAdminWarnings",
        component: () => import("../views/pages/WarehouseAdminWarnings.vue"),
      },
      {
        path: "logs",
        name: "warehouseAdminLogs",
        component: () => import("../views/pages/WarehouseAdminLogs.vue"),
      },
    ],
  },
  {
    path: "/preview/warehouse-admin",
    component: () => import("../views/WarehouseAdminLayout.vue"),
    children: [
      {
        path: "",
        redirect: "/preview/warehouse-admin/dashboard",
      },
      {
        path: "overview",
        redirect: "/preview/warehouse-admin/dashboard",
      },
      {
        path: "dashboard",
        name: "previewWarehouseAdminDashboard",
        component: () => import("../views/pages/WarehouseAdminDashboard.vue"),
      },
      {
        path: "create",
        name: "previewWarehouseAdminCreate",
        component: () => import("../views/pages/WarehouseAdminCreate.vue"),
      },
      {
        path: "warnings",
        name: "previewWarehouseAdminWarnings",
        component: () => import("../views/pages/WarehouseAdminWarnings.vue"),
      },
      {
        path: "logs",
        name: "previewWarehouseAdminLogs",
        component: () => import("../views/pages/WarehouseAdminLogs.vue"),
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
