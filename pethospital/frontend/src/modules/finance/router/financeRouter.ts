import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";
import { FINANCE_PORTAL_ROLES } from "@/core/auth/utils/roleUtils";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/finance",
    component: () => import("../views/FinanceLayout.vue"),
    meta: { requiresAuth: true, allowedRoles: [...FINANCE_PORTAL_ROLES] },
    children: [
      {
        path: "",
        redirect: "/finance/salary",
      },
      {
        path: "salary",
        name: "financeSalary",
        component: () => import("../views/pages/FinanceSalary.vue"),
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
