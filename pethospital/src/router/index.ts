import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";
import userRouters from "./userRouter"; // 引入用户模块的路由
import { store } from "@/store/userStore";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/PetHospitalHome",
    name: "PetHospitalHome",
    component: () => import("../App.vue"),
  },

  // 用户模块
  ...userRouters,
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});

// 路由守卫
router.beforeEach((to, from, next) => {
  // to: 目标路由信息
  // from: 当前路由信息
  // next: 控制路由跳转的函数

  // 只对需要认证的路由进行检查
  if (to.matched.some((record) => record.meta.requiresAuth)) {
    // 先检查本地 Vuex 状态，避免频繁连接 WebSocket
    if (store.state.auth.isLoggedIn) {
      next(); // 用户已登录，允许访问
    } else {
      // 只有在 Vuex 中未登录时才尝试连接 WebSocket 检查真实状态
      store
        .dispatch("auth/checkLoginStatus")
        .then(() => {
          // 检查用户是否已登录
          if (store.state.auth.isLoggedIn) {
            next(); // 用户已登录，允许访问
          } else {
            next({
              name: "PetHospitalHome",
              query: { redirect: to.fullPath },
              // 重定向到登录页
            });
          }
        })
        .catch((error) => {
          console.error("Failed to check login status:", error);
          // 即使检查失败也允许访问非关键页面
          next();
        });
    }
  } else {
    // 不需要认证的路由直接允许访问
    next();
  }
});

/*beforeEach：
Vue Router 提供的全局前置守卫，能在每次路由跳转之前检查条件。

to.meta.requiresAuth：
我们为需要认证的页面添加了 meta 字段，来标记该路由是否需要用户登录。通过 to.meta.requiresAuth 可以获取这个标记。

store.dispatch('checkLoginStatus')：
我们通过 Vuex 的 checkLoginStatus 动作来检查用户是否已登录。

next({ name: 'login' })：
如果用户未登录且试图访问需要登录的页面，则重定向到登录页面。*/
export default router;
