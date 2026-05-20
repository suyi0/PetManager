<template>
  <div class="shell">
    <aside class="sidebar">
      <div class="sidebar-head">{{ currentRoleLabel }}</div>
      <div class="sidebar-logo"></div>
      <nav>
        <RouterLink :to="`${routePrefix}/salary`">工资管理</RouterLink>
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
        <h2>宠物医院财务端</h2>
        <span>工资核算 · 成本归档 · 收益观察</span>
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
import { isFinancePortalRole } from "@/core/auth/utils/roleUtils";
import {
  startFinanceSessionGuard,
  stopFinanceSessionGuard,
} from "@/modules/finance/utils/financeSessionGuard";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";

export default defineComponent({
  name: "FinanceLayout",
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const { showBossReturn, returnToBossPortal } = useBossPortalReturn(router);

    onMounted(() => {
      startFinanceSessionGuard(store, router);
      void store.dispatch("superAdmin/ensureSalaryManagement");
    });

    onBeforeUnmount(() => {
      stopFinanceSessionGuard();
    });

    const logout = () => {
      stopFinanceSessionGuard();
      void store.dispatch("auth/logout");
    };

    const routePrefix = computed(() => "/finance");

    const currentRoleLabel = computed(() => {
      const activeRole = store.state.auth.userRole;
      return activeRole && isFinancePortalRole(activeRole)
        ? activeRole
        : "财务总监";
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
  position: sticky;
  display: flex;
  flex-direction: column;
  padding: 32px 20px 24px;
  border-right: 1px solid #d5e2ff;
  background: linear-gradient(180deg, #ffffff 0%, #f7faff 100%);
}

.sidebar-head {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 72px;
  margin-bottom: 18px;
  padding: 0 18px;
  border-radius: 28px;
  font-weight: 700;
  font-size: 30px;
  letter-spacing: 0.04em;
  color: #17345f;
  background: linear-gradient(135deg, #61d9c6 0%, #3ea3d4 100%);
  border: 1px solid rgba(52, 144, 196, 0.24);
}

.sidebar-logo {
  height: 84px;
  margin-bottom: 18px;
  border-radius: 28px;
  background: linear-gradient(
    135deg,
    rgba(90, 206, 201, 0.18),
    rgba(40, 130, 204, 0.06)
  );
  border: 1px solid rgba(116, 154, 222, 0.18);
}

nav {
  display: grid;
  gap: 12px;
}

nav a {
  display: flex;
  align-items: center;
  min-height: 68px;
  padding: 0 22px;
  border-radius: 22px;
  border: 1px solid #d7e4ff;
  background: #fff;
  color: #2c4778;
  font-size: 16px;
  font-weight: 600;
  text-decoration: none;
  box-shadow: 0 16px 30px rgba(69, 106, 178, 0.06);
}

nav a.router-link-active {
  background: linear-gradient(135deg, #eaf7ff 0%, #eff6ff 100%);
  color: #1c4f91;
  border-color: rgba(73, 125, 214, 0.26);
}

.boss-return {
  margin-top: auto;
  min-height: 56px;
  padding: 0 20px;
  border: 1px solid rgba(73, 125, 214, 0.24);
  border-radius: 20px;
  background: linear-gradient(135deg, #fff7e8 0%, #eaf8ff 100%);
  color: #1c4f91;
  cursor: pointer;
  font-size: 15px;
  font-weight: 800;
  text-align: left;
  box-shadow: 0 16px 30px rgba(69, 106, 178, 0.08);
  transition: transform 0.18s ease, box-shadow 0.18s ease;
}

.boss-return:hover {
  transform: translateY(-1px);
  box-shadow: 0 20px 34px rgba(69, 106, 178, 0.12);
}

.content {
  display: grid;
  align-content: start;
  gap: 20px;
  padding: 26px 30px 30px;
}

.topbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 18px 24px;
  border: 1px solid #d8e5ff;
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.86);
  box-shadow: 0 18px 34px rgba(56, 90, 152, 0.08);
}

.topbar h2 {
  margin: 0;
  font-size: 24px;
  color: #14284f;
}

.topbar span {
  color: #6c7da1;
}

.lgout {
  border: 0;
  border-radius: 999px;
  padding: 12px 18px;
  background: linear-gradient(135deg, #59d8c5, #3aa4d2);
  color: #fff;
  font-weight: 700;
  cursor: pointer;
}

@media (max-width: 1100px) {
  .shell {
    grid-template-columns: 1fr;
  }

  .sidebar {
    position: static;
    height: auto;
  }
}
</style>
