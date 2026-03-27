import type { Router } from "vue-router";
import type { Store } from "vuex";
import { superAdminApi } from "@/modules/super-admin/api/superAdminApi";
import { authStorage } from "@/core/auth/utils/authStorage";
import type { State } from "@/store/appStore";

const IDLE_TIMEOUT_MS = 5 * 60 * 1000; // 空闲超时时间
const REFRESH_THRESHOLD_MS = 2 * 60 * 1000; // 刷新阀值时间
const CHECK_INTERVAL_MS = 15 * 1000; // 检查间隔时间

// 监听的事件
const ACTIVITY_EVENTS = [
  "mousemove", // 移动鼠标
  "mousedown", // 按下鼠标
  "keydown", // 添加按键事件
  "scroll", // 添加滚动事件
  "touchstart", // 添加触摸事件
] as const;

let started = false; // 是否已启动
let lastActivityAt = 0; // 上次活动时间
let intervalId: number | null = null; // 定时器 ID
let refreshPromise: Promise<void> | null = null; // 刷新令牌-可以接收Promise对象或者null
let boundStore: Store<State> | null = null; // 绑定的 store
let boundRouter: Router | null = null; // 绑定的 router

// boundRouter.currentRoute.value.path 获取当前路由路径
// startsWith 方法检查是否为超级管理员路由
const isAdminRoute = () =>
  Boolean(boundRouter?.currentRoute.value.path.startsWith("/super-admin"));

// 检查管理员会话是否活动
const isAdminSessionActive = () =>
  Boolean(
    boundStore &&
      boundStore.state.auth.isLoggedIn &&
      boundStore.state.auth.userRole === "超级管理员" &&
      isAdminRoute()
  );

// 记录活动的时间
const recordActivity = () => {
  lastActivityAt = Date.now();
};

// 强制管理员登出
const forceAdminLogout = async (reason: string) => {
  if (!boundStore || !boundRouter) {
    // 如果store或router未绑定，说明管理员登录状态未激活
    return;
  }

  const store = boundStore;
  const router = boundRouter;
  stopSuperAdminSessionGuard();
  store.commit("auth/clearSession"); // 清除管理员会话
  store.commit("currentUser/clearCurrentUser");

  await router.replace({
    name: "PetHospital",
    query: { reason }, // 添加原因
  });
};

// 刷新管理员令牌
const refreshAdminTokenIfNeeded = async () => {
  if (!boundStore || refreshPromise || !isAdminSessionActive()) {
    return;
  }

  // 获取令牌剩余时间
  const remainingMs = authStorage.getTokenRemainingMs();
  if (remainingMs === null || remainingMs > REFRESH_THRESHOLD_MS) {
    return;
  }

  refreshPromise = (async () => {
    try {
      const response = await superAdminApi.refreshAdminSession();
      const refreshData = response.data?.data ?? response.data;

      if (response.status === 200 && refreshData?.token) {
        boundStore?.commit("auth/refreshToken", refreshData.token);
      }

      return;
    } catch {
      boundStore?.commit("auth/clearSession");
      boundStore?.commit("currentUser/clearCurrentUser");
      await forceAdminLogout("admin-session-refresh-failed");
    } finally {
      refreshPromise = null;
    }
  })();

  await refreshPromise;
};

// 检查管理员会话
const checkAdminSession = async () => {
  if (!isAdminSessionActive()) {
    return;
  }

  const token = authStorage.getToken();
  if (!token) {
    await forceAdminLogout("admin-token-expired");
    return;
  }

  if (Date.now() - lastActivityAt >= IDLE_TIMEOUT_MS) {
    await forceAdminLogout("admin-idle-timeout");
    return;
  }

  await refreshAdminTokenIfNeeded();
};

// 添加活动监听器
const attachActivityListeners = () => {
  ACTIVITY_EVENTS.forEach((eventName) => {
    // { passive: true } 表示事件处理程序不会调用 preventDefault() 方法
    window.addEventListener(eventName, recordActivity, { passive: true });
  });
};

// 移除活动监听器
const detachActivityListeners = () => {
  ACTIVITY_EVENTS.forEach((eventName) => {
    window.removeEventListener(eventName, recordActivity);
  });
};

// 启动管理员会话保护
export const startSuperAdminSessionGuard = (
  store: Store<State>,
  router: Router
) => {
  boundStore = store;
  boundRouter = router;

  if (started) {
    recordActivity();
    return;
  }

  started = true;
  recordActivity();
  attachActivityListeners();
  // window.setInterval(回调函数, 时间间隔) - 设置一个定时器，每隔一段时间执行一次回调函数
  intervalId = window.setInterval(() => {
    void checkAdminSession();
  }, CHECK_INTERVAL_MS);
};

// 停止管理员会话保护
export const stopSuperAdminSessionGuard = () => {
  if (intervalId !== null) {
    // window.clearInterval(定时器ID) - 清除定时器
    window.clearInterval(intervalId);
    intervalId = null;
  }

  detachActivityListeners();
  refreshPromise = null;
  started = false;
  boundStore = null;
  boundRouter = null;
};
