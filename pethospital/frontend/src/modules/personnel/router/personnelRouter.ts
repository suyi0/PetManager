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
    ],
  },
  {
    path: "/preview/personnel",
    component: () => import("../views/PersonnelLayout.vue"),
    children: [
      {
        path: "",
        redirect: "/preview/personnel/access",
      },
      {
        path: "access",
        name: "previewPersonnelAccess",
        component: () => import("../views/pages/PersonnelRoleAccess.vue"),
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
