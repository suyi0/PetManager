import { createApp } from "vue";
import App from "./App.vue";
import "./registerServiceWorker";
import router from "../src/router";
import { appStore, storeKey } from "@/store/appStore";
import { authStorage } from "@/core/auth/utils/authStorage";

// 初始化预约数据
appStore.dispatch("reservation/initReservateData");

// 如果用户已登录，启动每日更新预约表单的定时器
if (appStore.state.auth.isLoggedIn) {
  appStore.dispatch("reservation/scheduleDailyUpdate");
}

createApp(App)
  .provide(storeKey, appStore)
  .use(appStore)
  .use(router)
  .mount("#app");

// 在 main.ts 或 App.vue 中
window.addEventListener("beforeunload", () => {
  // 在页面关闭前保存用户数据
  if (appStore.state.auth.isLoggedIn) {
    const token = authStorage.getToken();
    if (!token) return;

    void fetch("/api/upload/form", {
      method: "POST",
      keepalive: true,
      headers: {
        "Content-Type": "application/json",
        Authorization: `Bearer ${token}`,
      },
      body: JSON.stringify({
        name: appStore.state.currentUser.userName,
        phone: appStore.state.currentUser.userPhone,
        email: appStore.state.currentUser.userEmail,
        birthday: appStore.state.currentUser.userBirthday,
        address: appStore.state.currentUser.userAddress,
      }),
    });
  }
});
