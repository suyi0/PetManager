import type { Router } from "vue-router";
import type { Store } from "vuex";
import { superAdminApi } from "@/modules/super-admin/api/superAdminApi";
import { authStorage } from "@/core/auth/utils/authStorage";
import { isWarehousePortalRole } from "@/core/auth/utils/roleUtils";
import type { State } from "@/app/store";

const IDLE_TIMEOUT_MS = 5 * 60 * 1000;
const REFRESH_THRESHOLD_MS = 2 * 60 * 1000;
const CHECK_INTERVAL_MS = 15 * 1000;

const ACTIVITY_EVENTS = [
  "mousemove",
  "mousedown",
  "keydown",
  "scroll",
  "touchstart",
] as const;

let started = false;
let lastActivityAt = 0;
let intervalId: number | null = null;
let refreshPromise: Promise<void> | null = null;
let boundStore: Store<State> | null = null;
let boundRouter: Router | null = null;

const isWarehouseRoute = () =>
  Boolean(boundRouter?.currentRoute.value.path.startsWith("/warehouse-admin"));

const isWarehouseSessionActive = () =>
  Boolean(
    boundStore?.state.auth.isLoggedIn &&
      isWarehousePortalRole(boundStore.state.auth.userRole) &&
      isWarehouseRoute()
  );

const recordActivity = () => {
  lastActivityAt = Date.now();
};

const forceWarehouseLogout = async (reason: string) => {
  if (!boundStore || !boundRouter) {
    return;
  }

  stopWarehouseAdminSessionGuard();
  boundStore.commit("auth/clearSession");
  boundStore.commit("currentUser/clearCurrentUser");

  await boundRouter.replace({
    name: "PetHospital",
    query: { reason },
  });
};

const refreshAdminTokenIfNeeded = async () => {
  if (!boundStore || refreshPromise || !isWarehouseSessionActive()) {
    return;
  }

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
    } catch {
      boundStore?.commit("auth/clearSession");
      boundStore?.commit("currentUser/clearCurrentUser");
      await forceWarehouseLogout("warehouse-session-refresh-failed");
    } finally {
      refreshPromise = null;
    }
  })();

  await refreshPromise;
};

const checkWarehouseSession = async () => {
  if (!isWarehouseSessionActive()) {
    return;
  }

  const token = authStorage.getToken();
  if (!token) {
    await forceWarehouseLogout("warehouse-token-expired");
    return;
  }

  if (Date.now() - lastActivityAt >= IDLE_TIMEOUT_MS) {
    await forceWarehouseLogout("warehouse-idle-timeout");
    return;
  }

  await refreshAdminTokenIfNeeded();
};

const attachActivityListeners = () => {
  ACTIVITY_EVENTS.forEach((eventName) => {
    window.addEventListener(eventName, recordActivity, { passive: true });
  });
};

const detachActivityListeners = () => {
  ACTIVITY_EVENTS.forEach((eventName) => {
    window.removeEventListener(eventName, recordActivity);
  });
};

export const startWarehouseAdminSessionGuard = (
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
  intervalId = window.setInterval(() => {
    void checkWarehouseSession();
  }, CHECK_INTERVAL_MS);
};

export const stopWarehouseAdminSessionGuard = () => {
  if (intervalId !== null) {
    window.clearInterval(intervalId);
    intervalId = null;
  }

  detachActivityListeners();
  refreshPromise = null;
  started = false;
  boundStore = null;
  boundRouter = null;
};
