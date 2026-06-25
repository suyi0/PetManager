import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";
import { PERSONNEL_PORTAL_ROLES } from "@/core/auth/utils/roleUtils";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/personnel",
    component: () => import("../views/PersonnelLayout.vue"),
    meta: { requiresAuth: true, allowedRoles: [...PERSONNEL_PORTAL_ROLES] },
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
