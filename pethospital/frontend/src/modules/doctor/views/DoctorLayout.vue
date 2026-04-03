<template>
  <div class="shell">
    <aside class="sidebar">
      <div class="brand">
        <span class="eyebrow">Doctor Center</span>
        <h1>医生工作台</h1>
      </div>

      <nav class="nav">
        <RouterLink :to="`${basePath}/home`">工作台</RouterLink>
        <RouterLink :to="`${basePath}/queue`">待接诊队列</RouterLink>
        <RouterLink :to="`${basePath}/reservations`">预约订单</RouterLink>
        <RouterLink :to="`${basePath}/order-records`">订单记录</RouterLink>
        <RouterLink :to="`${basePath}/drafts`">诊单草稿</RouterLink>
      </nav>

      <div v-if="showAdminReturn" class="sidebar-footer">
        <button type="button" class="admin-return" @click="returnToSuperAdmin">
          返回超级管理员
        </button>
      </div>
    </aside>

    <main class="content">
      <RouterView />
    </main>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent } from "vue";
import { useRoute, useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";
import { authStorage } from "@/core/auth/utils/authStorage";

export default defineComponent({
  name: "DoctorLayout",
  setup() {
    const store = useStore(storeKey);
    const route = useRoute();
    const router = useRouter();

    const basePath = computed(() =>
      route.path.startsWith("/preview/doctor") ? "/preview/doctor" : "/doctor"
    );
    const adminBridge = computed(() => authStorage.loadAdminPortalBridge());
    const showAdminReturn = computed(() => Boolean(adminBridge.value));

    const returnToSuperAdmin = async () => {
      const bridge = adminBridge.value;
      if (!bridge) {
        return;
      }

      store.commit("auth/setSession", {
        token: bridge.token,
        userType: bridge.userType,
        userRole: bridge.userRole,
      });
      store.commit(
        "currentUser/setCurrentUser",
        {
          userType: bridge.userType,
          userRole: bridge.userRole,
          userName: bridge.userName,
          userPhone: bridge.userPhone,
          userEmail: bridge.userEmail,
          userBirthday: bridge.userBirthday,
          userAddressId: bridge.userAddressId,
          userAddress: bridge.userAddress || "",
          userHeadImage: bridge.userHeadImage || "",
        },
        { root: true }
      );
      authStorage.clearAdminPortalBridge();
      await router.push(bridge.returnTo);
    };

    return { basePath, showAdminReturn, returnToSuperAdmin };
  },
});
</script>

<style scoped>
.shell {
  display: grid;
  width: 100%;
  grid-template-columns: 272px minmax(0, 1fr);
  height: 100vh;
  min-height: 100vh;
  align-items: stretch;
  overflow: hidden;
  overflow-x: hidden;
  background: radial-gradient(
      circle at 85% 8%,
      rgba(210, 236, 229, 0.92),
      transparent 22%
    ),
    radial-gradient(
      circle at 8% 88%,
      rgba(217, 239, 235, 0.82),
      transparent 24%
    ),
    linear-gradient(180deg, #f7f3ea 0%, #edf4f1 52%, #eef3f7 100%);
  color: #18373a;
  font-family: "PingFang SC", "Noto Sans SC", sans-serif;
}

.sidebar {
  position: relative;
  display: flex;
  flex-direction: column;
  align-self: stretch;
  padding: 32px 20px 24px;
  border-right: 1px solid rgba(143, 173, 165, 0.26);
  background: linear-gradient(
    180deg,
    rgba(255, 251, 245, 0.9),
    rgba(248, 252, 250, 0.82)
  );
  backdrop-filter: blur(18px);
  box-shadow: inset -1px 0 0 rgba(255, 255, 255, 0.54);
  z-index: 9999;
}

.brand {
  position: relative;
  margin-bottom: 26px;
  padding: 22px 18px 20px;
  border-radius: 28px;
  border: 1px solid rgba(145, 176, 167, 0.28);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.78), #f6fbf8);
  box-shadow: 0 18px 38px rgba(45, 90, 86, 0.08);
}

.brand::after {
  content: "";
  position: absolute;
  right: 20px;
  top: 20px;
  width: 46px;
  height: 46px;
  border-radius: 16px;
  background: linear-gradient(135deg, #cfe9df, #9fc5bb);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.8);
}

.eyebrow {
  margin: 0 0 6px;
  font-size: 10px;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: #7c928c;
}

.brand h1,
.topbar h2 {
  margin: 0;
}

.brand h1 {
  position: relative;
  z-index: 1;
  max-width: 150px;
  font-size: 28px;
  line-height: 1.16;
  letter-spacing: 0.02em;
}

.nav {
  display: grid;
  gap: 12px;
  padding: 14px;
  border-radius: 28px;
  border: 1px solid rgba(146, 177, 168, 0.22);
  background: rgba(255, 255, 255, 0.68);
  box-shadow: 0 18px 38px rgba(42, 84, 80, 0.06);
}

.sidebar-footer {
  margin-top: auto;
  display: grid;
  gap: 10px;
  padding: 14px;
  border-radius: 24px;
  border: 1px solid rgba(146, 177, 168, 0.22);
  background: rgba(255, 255, 255, 0.72);
  box-shadow: 0 18px 38px rgba(42, 84, 80, 0.06);
}

.sidebar-footer__label {
  margin: 0;
  font-size: 11px;
  letter-spacing: 0.08em;
  color: #7a928c;
}

.nav a {
  position: relative;
  text-decoration: none;
  color: #486762;
  padding: 15px 16px 15px 20px;
  border-radius: 18px;
  border: 1px solid rgba(174, 197, 189, 0.3);
  background: linear-gradient(180deg, #fffdf9, #f5faf7);
  transition: 0.2s ease;
  box-shadow: 0 10px 22px rgba(56, 98, 94, 0.05);
  font-weight: 600;
}

.nav a::before {
  content: "";
  position: absolute;
  left: 10px;
  top: 50%;
  width: 5px;
  height: calc(100% - 22px);
  border-radius: 999px;
  transform: translateY(-50%);
  background: linear-gradient(180deg, #8fb5a6, #4f7570);
  opacity: 0;
}

.nav a:hover {
  transform: translateX(2px);
  border-color: rgba(109, 149, 140, 0.34);
  box-shadow: 0 14px 26px rgba(46, 90, 85, 0.08);
}

.nav a.router-link-active {
  color: #143739;
  border-color: rgba(93, 135, 127, 0.38);
  background: linear-gradient(135deg, #ebf8f3, #dbece5 68%, #f9f5ed);
  box-shadow: 0 16px 28px rgba(63, 98, 92, 0.12);
}

.nav a.router-link-active::before {
  opacity: 1;
}

.admin-return {
  border: 1px solid rgba(115, 157, 149, 0.34);
  border-radius: 18px;
  padding: 14px 16px;
  background: linear-gradient(135deg, #ecf7f1, #f9f4ea);
  color: #20484b;
  font-weight: 700;
  text-align: left;
  cursor: pointer;
  box-shadow: 0 14px 28px rgba(63, 98, 92, 0.1);
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.admin-return:hover {
  transform: translateY(-1px);
  box-shadow: 0 18px 32px rgba(63, 98, 92, 0.14);
}

.content {
  min-width: 0;
  height: 100vh;
  min-height: 100vh;
  overflow-x: hidden;
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  padding: 26px 28px 34px;
  box-sizing: border-box;
}

@media (max-width: 980px) {
  .shell {
    height: auto;
    grid-template-columns: 1fr;
    overflow: visible;
  }

  .sidebar {
    position: relative;
    height: auto;
    min-height: auto;
    border-right: 0;
    border-bottom: 1px solid rgba(143, 173, 165, 0.26);
  }

  .content {
    height: auto;
    min-height: 0;
    overflow-y: visible;
    padding: 18px;
  }
}

@media (max-width: 640px) {
  .brand h1 {
    max-width: none;
    font-size: 24px;
  }

  .nav {
    padding: 10px;
  }

  .page-intro {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
