<template>
  <div class="shell">
    <aside class="sidebar">
      <div class="sidebar-head">{{ currentRoleLabel }}</div>
      <div class="sidebar-logo"></div>
      <nav>
        <RouterLink :to="`${routePrefix}/overview`">总览</RouterLink>
        <RouterLink :to="`${routePrefix}/worktime`">考勤管理</RouterLink>
        <RouterLink :to="`${routePrefix}/users`">用户管理</RouterLink>
        <RouterLink :to="`${routePrefix}/online-doctors`">在线医生</RouterLink>
        <RouterLink :to="`${routePrefix}/logs`">日志审计</RouterLink>
      </nav>
      <button
        v-if="showBossReturn"
        type="button"
        class="boss-return"
        @click="returnToBossPortal"
      >
        返回总裁端
      </button>
    </aside>

    <main class="content">
      <header class="topbar">
        <h2>宠物医院管理端</h2>
        <span>接口驱动 · Crow + MySQL</span>
        <button @click="logout" class="lgout"><span>登出</span></button>
      </header>
      <RouterView />
    </main>
  </div>
</template>

<script lang="ts">
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { computed, defineComponent, onBeforeUnmount, onMounted } from "vue";
import { useRouter } from "vue-router";
import { isSuperAdminPortalRole } from "@/core/auth/utils/roleUtils";
import {
  startSuperAdminSessionGuard,
  stopSuperAdminSessionGuard,
} from "@/modules/super-admin/utils/superAdminSessionGuard";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";

export default defineComponent({
  name: "SuperAdminLayout",
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const { showBossReturn, returnToBossPortal } = useBossPortalReturn(router);

    onMounted(() => {
      startSuperAdminSessionGuard(store, router);
      // 超级管理员进入任意页面时，先从后端刷新核心业务数据并写入本地缓存。
      void store.dispatch("superAdmin/refreshSuperAdminData");
    });

    onBeforeUnmount(() => {
      stopSuperAdminSessionGuard();
    });

    // 登出
    const logout = () => {
      stopSuperAdminSessionGuard();
      if (store) {
        store.dispatch("auth/logout");
      } else {
        console.log("store is not defined");
      }
    };

    const routePrefix = computed(() => "/super-admin");

    const currentRoleLabel = computed(() => {
      const activeRole = store.state.auth.userRole;
      return activeRole && isSuperAdminPortalRole(activeRole)
        ? activeRole
        : "超级管理员";
    });

    return {
      logout,
      routePrefix,
      currentRoleLabel,
      showBossReturn,
      returnToBossPortal,
    };
  },
});
</script>

<style scoped>
.shell {
  display: grid;
  grid-template-columns: 268px 1fr;
  min-height: 100vh;
  background: radial-gradient(
    circle at 20% 10%,
    #f5f9ff 0%,
    #f0f4ff 45%,
    #e9eefb 100%
  );
  color: #182442;
  font-family: "PingFang SC", "Segoe UI", sans-serif;
}

.sidebar {
  top: 0;
  height: 100vh;
  min-height: 100vh;
  position: sticky;
  display: flex;
  flex-direction: column;
  align-self: stretch;
  padding: 32px 20px 24px;
  border-right: 1px solid #d5e2ff;
  background: linear-gradient(180deg, #ffffff 0%, #f7faff 100%);
  box-shadow: inset -1px 0 0 rgba(255, 255, 255, 0.8);
  z-index: 9999;

  .sidebar-head {
    display: flex;
    align-items: center;
    justify-content: center;
    min-height: 72px;
    margin-bottom: 18px;
    padding: 0 18px;
    font-weight: 700;
    font-size: 30px;
    letter-spacing: 0.04em;
    color: #17345f;
    background: linear-gradient(135deg, #4fd4d6 0%, #2ca8d5 100%);
    border: 1px solid rgba(52, 144, 196, 0.24);
    border-radius: 26px;
    box-shadow: 0 16px 30px rgba(45, 125, 186, 0.16);
  }
}

.sidebar-logo {
  height: 1px;
  margin: 0 8px 18px;
  background: linear-gradient(
    90deg,
    rgba(90, 122, 178, 0),
    rgba(90, 122, 178, 0.42),
    rgba(90, 122, 178, 0)
  );
}

.sidebar nav {
  display: grid;
  gap: 10px;
  padding: 12px;
  border: 1px solid #d9e5fb;
  border-radius: 22px;
  background: linear-gradient(180deg, rgba(245, 248, 255, 0.96), #ffffff);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 14px 28px rgba(97, 122, 168, 0.08);
}

.sidebar a {
  position: relative;
  text-decoration: none;
  color: #415177;
  padding: 14px 16px 14px 18px;
  border: 1px solid #e4ebfb;
  border-radius: 16px;
  background: linear-gradient(180deg, #ffffff, #f9fbff);
  font-size: 15px;
  font-weight: 600;
  transition: 0.2s ease;
  box-shadow: 0 8px 18px rgba(117, 138, 181, 0.06);
}

.sidebar a::before {
  content: "";
  position: absolute;
  left: 10px;
  top: 50%;
  width: 4px;
  height: calc(100% - 20px);
  border-radius: 999px;
  background: linear-gradient(180deg, #8fb5ff, #bdd0ff);
  opacity: 0;
  transform: translateY(-50%);
  transition: 0.2s ease;
}

.sidebar a:hover {
  color: #243559;
  border-color: #cfdcf7;
  transform: translateX(2px);
  box-shadow: 0 12px 24px rgba(94, 124, 177, 0.12);
}

.sidebar a.router-link-active {
  color: #16335c;
  border-color: rgba(85, 146, 232, 0.34);
  background: linear-gradient(135deg, #e8f7ff 0%, #dff1ff 52%, #eef6ff 100%);
  box-shadow: 0 16px 28px rgba(75, 126, 186, 0.16);
}

.sidebar a.router-link-active::before {
  opacity: 1;
}

.sidebar a + a {
  margin-top: 2px;
}

.boss-return {
  width: calc(100% - 24px);
  margin: auto 12px 0;
  padding: 14px 16px;
  border: 1px solid rgba(72, 120, 198, 0.28);
  border-radius: 18px;
  background: linear-gradient(135deg, #fff8e8, #eaf2ff);
  color: #1e4276;
  cursor: pointer;
  font-size: 15px;
  font-weight: 800;
  text-align: left;
  box-shadow: 0 14px 26px rgba(78, 112, 178, 0.12);
  transition: transform 0.18s ease, box-shadow 0.18s ease;
}

.boss-return:hover {
  transform: translateY(-1px);
  box-shadow: 0 18px 32px rgba(78, 112, 178, 0.16);
}

.content {
  min-width: 0;
  height: 100vh;
  min-height: 100vh;
  overflow-x: hidden;
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  padding: 20px 26px 32px;
  box-sizing: border-box;
}

.topbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 18px;
}

.topbar h2 {
  margin: 0;
  font-size: 24px;
}

.topbar span {
  font-size: 12px;
  color: #6c7c9f;
}

.lgout {
  border-radius: 999px;
  background: linear-gradient(135deg, #9debe4, #19aaaf);

  span {
    font-size: 18px;
  }
}

@media (max-width: 960px) {
  .shell {
    grid-template-columns: 1fr;
  }

  .sidebar {
    top: auto;
    height: auto;
    min-height: auto;
    position: relative;
    border-right: 0;
    border-bottom: 1px solid #d5e2ff;
  }

  .sidebar nav {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .content {
    height: auto;
    min-height: 0;
    overflow-y: visible;
  }
}

@media (max-width: 640px) {
  .sidebar nav {
    grid-template-columns: 1fr;
  }
}
</style>
