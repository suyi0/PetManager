import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/boss",
    component: () => import("../views/BossLayout.vue"),
    meta: {
      requiresAuth: true,
      allowedPermissions: ["portal:boss"],
    },
    children: [
      {
        path: "",
        redirect: "/boss/overview",
      },
      {
        path: "overview",
        name: "bossOverview",
        component: () => import("../views/pages/BossOverview.vue"),
      },
      {
        path: "personal",
        name: "bossPersonal",
        component: () =>
          import(
            "@/modules/user/views/desktop/Personal-module/UserPersonalView.vue"
          ),
        props: { roleBadge: "总裁端" },
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
