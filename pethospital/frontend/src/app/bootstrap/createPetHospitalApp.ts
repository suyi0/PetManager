import { createApp } from "vue";
import App from "@/App.vue";
import "@/registerServiceWorker";
import { HTTP_AUTH_EXPIRED_EVENT } from "@/api/httpError";
import router from "@/app/router";
import { appStore, storeKey } from "@/app/store";

const setupAuthExpiredListener = () => {
  if (typeof window === "undefined") {
    return;
  }

  window.addEventListener(HTTP_AUTH_EXPIRED_EVENT, () => {
    void appStore.dispatch("auth/expireSession");

    const currentRoute = router.currentRoute.value;
    if (currentRoute.name === "PetHospital") {
      return;
    }

    void router.replace({
      name: "PetHospital",
      query: {
        redirect: currentRoute.fullPath,
        reason: "session-expired",
      },
    });
  });
};

export const createPetHospitalApp = () => {
  setupAuthExpiredListener();

  return createApp(App).provide(storeKey, appStore).use(appStore).use(router);
};
