import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/personnel",
    component: () => import("../views/PersonnelLayout.vue"),
    meta: {
      requiresAuth: true,
      allowedPermissions: ["portal:personnel"],
    },
    children: [
      {
        path: "",
        redirect: "/personnel/employment",
      },
      {
        // 兼容旧入口「任职与薪酬」合并页
        path: "access",
        redirect: "/personnel/employment",
      },
      {
        path: "employment",
        name: "personnelEmployment",
        component: () => import("../views/pages/PersonnelEmployment.vue"),
      },
      {
        path: "compensation",
        name: "personnelCompensation",
        component: () => import("../views/pages/PersonnelCompensation.vue"),
      },
      {
        path: "personal",
        name: "personnelPersonal",
        component: () =>
          import(
            "@/modules/user/views/desktop/Personal-module/UserPersonalView.vue"
          ),
        props: { roleBadge: "人事端" },
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
