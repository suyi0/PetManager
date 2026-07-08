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
        redirect: "/personnel/access",
      },
      {
        path: "access",
        name: "personnelAccess",
        component: () => import("../views/pages/PersonnelRoleAccess.vue"),
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
