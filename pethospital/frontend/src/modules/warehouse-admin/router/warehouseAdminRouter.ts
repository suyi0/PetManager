import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/warehouse-admin",
    component: () =>
      import(
        "@/modules/warehouse-admin/views/desktop/WarehouseAdminLayout.vue"
      ),
    meta: {
      requiresAuth: true,
      allowedPermissions: ["portal:warehouse"],
    },
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
      {
        path: "personal",
        name: "warehouseAdminPersonal",
        component: () =>
          import(
            "@/modules/user/views/desktop/Personal-module/UserPersonalView.vue"
          ),
        props: { roleBadge: "仓库端" },
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
