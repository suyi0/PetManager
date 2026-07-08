import type { Router } from "vue-router";
import type { Store } from "vuex";
import { personnelApi } from "@/modules/personnel/api/personnelApi";
import { authStorage } from "@/core/auth/utils/authStorage";
import type { State } from "@/app/store";

const IDLE_TIMEOUT_MS = 30 * 60 * 1000;
const REFRESH_THRESHOLD_MS = 5 * 60 * 1000;
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

const isPersonnelRoute = () =>
  Boolean(boundRouter?.currentRoute.value.path.startsWith("/personnel"));

const isPersonnelSessionActive = () =>
  Boolean(
    boundStore?.state.auth.isLoggedIn &&
      boundStore.state.auth.permissions.includes("portal:personnel") &&
      isPersonnelRoute()
  );

const recordActivity = () => {
  lastActivityAt = Date.now();
};

const forcePersonnelLogout = async (reason: string) => {
  if (!boundStore || !boundRouter) {
    return;
  }

  stopPersonnelSessionGuard();
  boundStore.commit("auth/clearSession");
  boundStore.commit("currentUser/clearCurrentUser");

  await boundRouter.replace({
    name: "PetHospital",
    query: { reason },
  });
};

const refreshPersonnelTokenIfNeeded = async () => {
  if (!boundStore || refreshPromise || !isPersonnelSessionActive()) {
    return;
  }

  const remainingMs = authStorage.getTokenRemainingMs();
  if (remainingMs === null || remainingMs > REFRESH_THRESHOLD_MS) {
    return;
  }

  refreshPromise = (async () => {
    try {
      const response = await personnelApi.refreshSession();
      const refreshData = response.data?.data ?? response.data;

      if (response.status === 200 && refreshData?.token) {
        boundStore?.commit("auth/refreshToken", refreshData.token);
      }
    } catch {
      boundStore?.commit("auth/clearSession");
      boundStore?.commit("currentUser/clearCurrentUser");
      await forcePersonnelLogout("personnel-session-refresh-failed");
    } finally {
      refreshPromise = null;
    }
  })();

  await refreshPromise;
};

const checkPersonnelSession = async () => {
  if (!isPersonnelSessionActive()) {
    return;
  }

  const token = authStorage.getToken();
  if (!token) {
    await forcePersonnelLogout("personnel-token-expired");
    return;
  }

  if (Date.now() - lastActivityAt >= IDLE_TIMEOUT_MS) {
    await forcePersonnelLogout("personnel-idle-timeout");
    return;
  }

  await refreshPersonnelTokenIfNeeded();
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

export const startPersonnelSessionGuard = (
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
    void checkPersonnelSession();
  }, CHECK_INTERVAL_MS);
};

export const stopPersonnelSessionGuard = () => {
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
