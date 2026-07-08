import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/finance",
    component: () => import("../views/FinanceLayout.vue"),
    meta: {
      requiresAuth: true,
      allowedPermissions: ["portal:finance"],
    },
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
      {
        path: "personal",
        name: "financePersonal",
        component: () =>
          import(
            "@/modules/user/views/desktop/Personal-module/UserPersonalView.vue"
          ),
        props: { roleBadge: "财务端" },
      },
    ],
  },
];

export default createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});
