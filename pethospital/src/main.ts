import { createApp } from "vue";
import App from "./App.vue";
import "./registerServiceWorker";
import router from "./router";
import store, { key } from "./store";

createApp(App).use(store, key).use(router).mount("#app");

// 在 main.ts 或 App.vue 中
window.addEventListener("beforeunload", () => {
  // 在页面关闭前保存用户数据
  if (store.state.auth.isLoggedIn) {
    // 注意：由于浏览器限制，这里可能无法发送异步请求
    // 可以考虑使用 sendBeacon API
    navigator.sendBeacon(
      "/api/user/form",
      JSON.stringify({
        name: store.state.auth.userName,
        phone: store.state.auth.userPhone,
        email: store.state.auth.userEmail,
        birthday: store.state.auth.userBirthday,
        address: store.state.auth.userAddress,
      })
    );
  }
});
