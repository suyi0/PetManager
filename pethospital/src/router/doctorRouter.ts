import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/doctor/home",
    name: "doctorHome",
    component: () =>
      import("../views/DoctorDashboard/DoctorHome-module/DoctorHome.vue"),
  },
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});

export default router;
