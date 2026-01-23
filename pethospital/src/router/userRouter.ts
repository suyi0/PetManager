import { RouteRecordRaw } from "vue-router";

const userRoutes: Array<RouteRecordRaw> = [
  {
    path: "/user/register/account",
    name: "userRegisterAccount",
    component: () => import("../components/RegisterView.vue"),
  },
  {
    path: "/user/home",
    name: "userHome",
    component: () =>
      import("../views/UserDashboard/UserHome-module/UserHome.vue"),
    children: [
      {
        path: "about",
        name: "userAbout",
        component: () =>
          import(
            "../views/UserDashboard/UserHome-module/HomeNav/UserAbout.vue"
          ),
      },
      {
        path: "services",
        name: "userServices",
        component: () =>
          import(
            "../views/UserDashboard/UserHome-module/HomeNav/UserServices.vue"
          ),
      },
      {
        path: "/user/personal",
        name: "userPersonal",
        component: () =>
          import("../views/UserDashboard/Personal-module/UserPersonalView.vue"),
        meta: { requiresAuth: true }, // 需要认证才能访问
      },
      {
        path: "/user/order",
        component: () =>
          import(
            "../views/UserDashboard/Personal-module/order-module/orderView.vue"
          ),
        name: "userOrder",
        meta: { requiresAuth: true }, // 需要认证才能访问
      },
      {
        path: "/user/orderDetail",
        component: () =>
          import(
            "../views/UserDashboard/Personal-module/order-module/orderDetail.vue"
          ),
        name: "userOrderDetail",
        meta: { requiresAuth: true }, // 需要认证才能访问
      },
    ],
  },
];

export default userRoutes;
