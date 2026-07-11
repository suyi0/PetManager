<template>
  <div
    class="app-container"
    :class="{ 'app-container--workspace': isLoggedIn }"
  >
    <div v-if="!isLoggedIn" class="guest-shell">
      <div v-if="!showRegister" class="login-div">
        <section class="brand-panel">
          <div class="brand-panel__content">
            <div class="brand-panel__eyebrow">
              Pet Hospital Management System
            </div>
            <h1 class="brand-panel__title">宠物医院</h1>
            <div class="brand-panel_logo">
              <div class="brand-panel__logo-slot">
                <span class="brand-panel__logo-label">LOGO</span>
                <span class="brand-panel__logo-tip">商标预留区</span>
              </div>
            </div>
            <p class="brand-panel__subtitle">
              专业、洁净、可信赖的宠物诊疗服务空间
            </p>
          </div>
        </section>
        <section class="login-panel">
          <Login />
        </section>
      </div>
      <!-- 注册路由 -->
      <div v-else class="register-div">
        <router-view></router-view>
      </div>
    </div>
    <div v-if="isLoggedIn" class="logined-container">
      <div>
        <router-view></router-view>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import Login from "./views/LoginPage.vue";
import router from "@/app/router";
import { getHomeRouteByUserAccess } from "@/core/auth/utils/authRedirect";

const store = useStore(storeKey);

const isLoggedIn = computed(() => store.state.auth.isLoggedIn);
const showRegister = computed(() => store.state.ui.showRegister);

onMounted(async () => {
  if (isLoggedIn.value) {
    // 刷新页面后权限集可能未加载（不持久化），先拉 /auth/me 再按权限定首页
    if (!store.state.auth.permissions?.length) {
      await store.dispatch("auth/refreshAccess").catch(() => undefined);
    }
    await router.isReady();
    const currentPath = router.currentRoute.value.path;
    if (currentPath === "/" || currentPath === "/PetHospital") {
      await router.replace(getHomeRouteByUserAccess(store.state.auth));
    }
  }
});
</script>

<style scoped>
.app-container {
  box-sizing: border-box;
  position: relative;
  width: 100%;
  height: 100vh;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
  background: radial-gradient(
      circle at top left,
      rgba(62, 139, 255, 0.18),
      transparent 34%
    ),
    linear-gradient(135deg, #f8fbff 0%, #eef5ff 46%, #ffffff 100%);
}

.app-container--workspace {
  height: 100vh;
  min-height: 100vh;
  overflow-y: auto;
  overflow-x: hidden;
  -webkit-overflow-scrolling: touch;
  align-items: stretch;
  justify-content: flex-start;
}

.login-div {
  box-sizing: border-box;
  display: grid;
  position: absolute;
  top: 6vh;
  left: 6vw;
  grid-template-columns: minmax(320px, 0.92fr) minmax(420px, 1fr);
  align-items: center;
  width: min(1360px, calc(100vw - 88px));
  min-height: min(760px, calc(100vh - 88px));
  padding: 22px;
  border-radius: 36px;
  background: linear-gradient(
    135deg,
    rgba(255, 255, 255, 0.68) 0%,
    rgba(240, 246, 255, 0.9) 50%,
    rgba(255, 255, 255, 0.76) 100%
  );
  box-shadow: 0 28px 90px rgba(36, 85, 145, 0.12),
    inset 0 0 0 1px rgba(255, 255, 255, 0.62);
  backdrop-filter: blur(20px);
}

.brand-panel {
  --logo-slot-size: 132px;
  --logo-slot-margin: 28px 0 22px;
  position: relative;
  overflow: hidden;
  align-self: stretch;
  padding: 56px 46px 56px 54px;
  border-radius: 28px;
  background: linear-gradient(
    180deg,
    rgba(255, 255, 255, 0.66) 0%,
    rgba(239, 246, 255, 0.88) 100%
  );
}

.brand-panel::before,
.brand-panel::after {
  content: "";
  position: absolute;
  border-radius: 999px;
  pointer-events: none;
}

.brand-panel::before {
  top: -120px;
  left: -80px;
  width: 320px;
  height: 320px;
  background: radial-gradient(
    circle,
    rgba(30, 94, 255, 0.16) 0%,
    rgba(30, 94, 255, 0) 68%
  );
}

.brand-panel::after {
  right: -36px;
  bottom: -80px;
  width: 240px;
  height: 240px;
  background: radial-gradient(
    circle,
    rgba(103, 176, 255, 0.22) 0%,
    rgba(103, 176, 255, 0) 72%
  );
}

.brand-panel__content {
  position: relative;
  z-index: 1;
  display: flex;
  flex-direction: column;
  justify-content: center;
  height: 100%;
  max-width: 500px;
}

.brand-panel__eyebrow {
  margin-bottom: 18px;
  color: #4e77a5;
  font-size: 14px;
  letter-spacing: 0.22em;
  text-transform: uppercase;
}

.brand-panel__title {
  margin: 0;
  color: #103b73;
  font-size: clamp(48px, 6vw, 76px);
  line-height: 1.02;
  font-weight: 700;
  letter-spacing: 0.04em;
}

.brand-panel_logo {
  width: 80%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.brand-panel__logo-slot {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8px;
  width: var(--logo-slot-size);
  height: var(--logo-slot-size);
  margin: var(--logo-slot-margin);
  border: 1.5px dashed rgba(30, 94, 255, 0.35);
  border-radius: 28px;
  background: rgba(255, 255, 255, 0.72);
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.55);
}

.brand-panel__logo-label {
  color: #1e5eff;
  font-size: 22px;
  font-weight: 700;
  letter-spacing: 0.16em;
}

.brand-panel__logo-tip {
  color: #6e8fb5;
  font-size: 13px;
}

.brand-panel__subtitle {
  margin: 0;
  max-width: 360px;
  color: #466789;
  font-size: 17px;
  line-height: 1.75;
}

.login-panel {
  display: flex;
  align-items: center;
  justify-content: center;
  align-self: stretch;
  padding: 14px 10px 14px 18px;
  position: relative;
}

.login-panel::before {
  content: "";
  position: absolute;
  left: -18px;
  top: 8%;
  bottom: 8%;
  width: 1px;
  background: linear-gradient(
    180deg,
    rgba(130, 170, 224, 0) 0%,
    rgba(130, 170, 224, 0.28) 22%,
    rgba(130, 170, 224, 0.28) 78%,
    rgba(130, 170, 224, 0) 100%
  );
}

.register-div {
  width: 100%;
  min-height: 100%;
  height: auto;
  overflow: visible;
}

.guest-shell {
  width: 100%;
  min-height: 100%;
  height: auto;
  display: flex;
  align-items: stretch;
  justify-content: flex-start;
}

.logined-container {
  width: 100%;
  min-height: 100%;
  overflow: visible;
}

.guest-shell > .register-div,
.logined-container > div {
  min-height: 100%;
  overflow: visible;
}

@media (max-width: 960px) {
  .login-div {
    grid-template-columns: 1fr;
    width: min(100%, calc(100vw - 32px));
    min-height: auto;
    padding: 14px;
    overflow-y: auto;
  }

  .brand-panel {
    --logo-slot-size: 108px;
    --logo-slot-margin: 22px 0 18px;
    min-height: 34vh;
    padding: 36px 28px 20px;
  }

  .brand-panel__content {
    height: auto;
    max-width: none;
  }

  .login-panel {
    padding: 6px 4px 8px;
  }

  .login-panel::before {
    display: none;
  }
}

@media (max-width: 640px) {
  .app-container {
    align-items: stretch;
    width: 100%;
    min-height: 100vh;
  }

  .login-div {
    width: 100%;
    min-height: 100%;
    border-radius: 0;
    padding: 10px;
  }
}
</style>

<style>
html,
body,
#app {
  margin: 0;
  width: 100%;
  height: 100%;
  min-height: 100%;
  overflow: hidden;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}
</style>
