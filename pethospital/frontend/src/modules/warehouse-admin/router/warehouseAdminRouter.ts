import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";
import { WAREHOUSE_PORTAL_ROLES } from "@/core/auth/utils/roleUtils";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/warehouse-admin",
    component: () =>
      import(
        "@/modules/warehouse-admin/views/desktop/WarehouseAdminLayout.vue"
      ),
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
        component: () =>
          import(
            "@/modules/warehouse-admin/views/desktop/pages/WarehouseAdminDashboard.vue"
          ),
      },
      {
        path: "create",
        name: "warehouseAdminCreate",
        component: () =>
          import(
            "@/modules/warehouse-admin/views/desktop/pages/WarehouseAdminCreate.vue"
          ),
      },
      {
        path: "warnings",
        name: "warehouseAdminWarnings",
        component: () =>
          import(
            "@/modules/warehouse-admin/views/desktop/pages/WarehouseAdminWarnings.vue"
          ),
      },
      {
        path: "logs",
        name: "warehouseAdminLogs",
        component: () =>
          import(
            "@/modules/warehouse-admin/views/desktop/pages/WarehouseAdminLogs.vue"
          ),
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
