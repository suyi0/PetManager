import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/admin/home",
    name: "adminHome",
    component: () =>
      import("../views/AdminDashboard/AdminHome-module/AdminHome.vue"),
  },
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});

export default router;
