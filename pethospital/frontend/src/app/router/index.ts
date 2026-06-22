import {
  createRouter,
  createWebHashHistory,
  createWebHistory,
  RouteRecordRaw,
} from "vue-router";
import userRouters from "@/modules/user/router/UserRouter"; // 引入用户模块的路由
import adminRouters from "@/modules/super-admin/router/superAdminRouter"; // 引入管理员模块的路由
import warehouseAdminRouters from "@/modules/warehouse-admin/router/warehouseAdminRouter";
import doctorRouters from "@/modules/doctor/router/DoctorRouter";
import financeRouters from "@/modules/finance/router/financeRouter";
import personnelRouters from "@/modules/personnel/router/personnelRouter";
import bossRouters from "@/modules/boss/router/bossRouter";
import { appStore } from "@/app/store";
import { resolveRoleName } from "@/core/auth/utils/roleUtils";

const routes: Array<RouteRecordRaw> = [
  {
    path: "/PetHospital",
    name: "PetHospital",
    component: () => import("@/App.vue"),
    children: [
      {
        path: "/register/account",
        name: "userRegisterAccount",
        component: () => import("@/views/RegisterView.vue"),
      },
    ],
  },

  ...userRouters,
  ...doctorRouters,
  ...bossRouters.options.routes,
  ...financeRouters.options.routes,
  ...personnelRouters.options.routes,
  ...adminRouters.options.routes,
  ...warehouseAdminRouters.options.routes,
];

const router = createRouter({
  history:
    process.env.VUE_APP_DESKTOP_CLIENT === "true"
      ? createWebHashHistory()
      : createWebHistory(process.env.BASE_URL),
  routes,
});

const clearInvalidAuthState = () => {
  appStore.commit("auth/logout");
};

// 路由守卫
router.beforeEach((to, from, next) => {
  // to: 目标路由信息
  // from: 当前路由信息
  // next: 控制路由跳转的函数

  // 只对需要认证的路由进行检查
  if (to.matched.some((record) => record.meta.requiresAuth)) {
    const allowedRoles = to.matched
      .map((record) => record.meta.allowedRoles as string[] | undefined)
      .filter((roles): roles is string[] => Array.isArray(roles))
      .flat();

    // 确保当前用户角色匹配
    const ensureRoleMatches = () => {
      if (allowedRoles.length === 0) {
        next();
        return;
      }

      const currentUserRole = resolveRoleName(
        appStore.state.auth.userRole,
        appStore.state.auth.userType
      );
      if (currentUserRole && allowedRoles.includes(currentUserRole)) {
        next();
        return;
      }

      clearInvalidAuthState();
      next({
        name: "PetHospital",
        query: { redirect: to.fullPath },
      });
    };

    // 先检查本地 Vuex 状态，避免频繁连接 WebSocket
    if (appStore.state.auth.isLoggedIn) {
      ensureRoleMatches();
    } else {
      // 只有在 Vuex 中未登录时才尝试连接 WebSocket 检查真实状态
      appStore
        .dispatch("auth/checkLoginStatus")
        .then(() => {
          // 检查用户是否已登录
          if (appStore.state.auth.isLoggedIn) {
            ensureRoleMatches();
          } else {
            next({
              name: "PetHospital",
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

userStore.dispatch('checkLoginStatus')：
我们通过 Vuex 的 checkLoginStatus 动作来检查用户是否已登录。

next({ name: 'login' })：
如果用户未登录且试图访问需要登录的页面，则重定向到登录页面。*/
export default router;
