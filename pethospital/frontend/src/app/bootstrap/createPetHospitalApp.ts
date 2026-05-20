import { createApp } from "vue";
import App from "@/App.vue";
import "@/registerServiceWorker";
import router from "@/app/router";
import { appStore, storeKey } from "@/app/store";

export const createPetHospitalApp = () =>
  createApp(App).provide(storeKey, appStore).use(appStore).use(router);
