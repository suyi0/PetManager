<template>
  <div class="warehouse-stage">
    <div class="shell">
      <aside class="sidebar">
        <div class="brand-stack">
          <div class="logo-core">PM</div>
        </div>

        <nav>
          <RouterLink :to="`${routePrefix}/dashboard`">库存台账</RouterLink>
          <RouterLink :to="`${routePrefix}/create`">新增入库</RouterLink>
          <RouterLink :to="`${routePrefix}/warnings`">库存预警</RouterLink>
          <RouterLink :to="`${routePrefix}/logs`">日志</RouterLink>
        </nav>

        <button
          v-if="showBossReturn"
          type="button"
          class="portal-return"
          @click="returnToBossPortal"
        >
          返回总裁端
        </button>

        <button
          v-if="showAdminReturn"
          type="button"
          class="portal-return"
          @click="returnToSuperAdmin"
        >
          返回超级管理员
        </button>
      </aside>

      <main class="content">
        <header class="topbar">
          <p class="eyebrow">仓库管理</p>
          <div class="topbar-actions">
            <button type="button" @click="logout">登出</button>
          </div>
        </header>

        <section class="page-viewport">
          <RouterView />
        </section>
      </main>
    </div>
  </div>
</template>

<script lang="ts">
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { computed, defineComponent, onBeforeUnmount, onMounted } from "vue";
import { useRouter } from "vue-router";
import { authStorage } from "@/core/auth/utils/authStorage";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";
import {
  startWarehouseAdminSessionGuard,
  stopWarehouseAdminSessionGuard,
} from "@/modules/warehouse-admin/utils/warehouseAdminSessionGuard";

export default defineComponent({
  name: "WarehouseAdminLayout",
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const { showBossReturn, returnToBossPortal } = useBossPortalReturn(router);

    onMounted(() => {
      startWarehouseAdminSessionGuard(store, router);
    });

    onBeforeUnmount(() => {
      stopWarehouseAdminSessionGuard();
    });

    const logout = () => {
      stopWarehouseAdminSessionGuard();
      if (store) {
        store.dispatch("auth/logout");
      }
    };

    const routePrefix = computed(() => "/warehouse-admin");

    const adminBridge = computed(() => authStorage.loadAdminPortalBridge());
    const showAdminReturn = computed(() => Boolean(adminBridge.value));

    const returnToSuperAdmin = async () => {
      const bridge = adminBridge.value;
      if (!bridge) {
        return;
      }

      await store.dispatch("auth/restoreAdminPortalBridgeSession", bridge);
      await router.push(bridge.returnTo);
    };

    return {
      logout,
      routePrefix,
      showAdminReturn,
      returnToSuperAdmin,
      showBossReturn,
      returnToBossPortal,
    };
  },
});
</script>

<style scoped>
.warehouse-stage {
  box-sizing: border-box;
  height: 100vh;
  min-height: 100vh;
  padding: 16px;
  overflow: hidden;
  background: #f6faf9;
}

.shell {
  display: grid;
  grid-template-columns: 176px minmax(0, 1fr);
  height: calc(100vh - 32px);
  min-height: 0;
  width: min(100%, 1536px);
  margin: 0 auto;
  overflow: clip;
  border-radius: 18px;
  border: 1px solid #dfe7df;
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(35, 62, 46, 0.06);
  color: #1d3429;
}

.sidebar {
  position: sticky;
  top: 0;
  display: flex;
  flex-direction: column;
  gap: 24px;
  min-width: 0;
  height: 100%;
  min-height: 0;
  padding: 18px 14px;
  border-right: 1px solid #dfe7df;
  background: #f4f7f4;
}

.brand-stack {
  display: grid;
  gap: 12px;
}

.logo-core {
  display: grid;
  place-items: center;
  width: 44px;
  height: 44px;
  border-radius: 12px;
  border: 1px solid rgba(36, 88, 73, 0.2);
  background: #245849;
  color: #ffffff;
  font-weight: 800;
  letter-spacing: 0;
}

nav {
  display: grid;
  gap: 10px;
}

.sidebar a {
  min-height: 44px;
  box-sizing: border-box;
  padding: 13px 12px;
  border-radius: 8px;
  color: #1d3429;
  text-decoration: none;
  font-size: 12px;
  font-weight: 700;
  border: 1px solid transparent;
  background: transparent;
  transition: background-color 160ms ease, color 160ms ease,
    border-color 160ms ease;
}

.sidebar a:hover,
.sidebar a.router-link-active {
  color: #ffffff;
  border-color: #245849;
  background: #245849;
}

.sidebar a:focus-visible,
button:focus-visible {
  outline: 3px solid rgba(36, 88, 73, 0.24);
  outline-offset: 2px;
}

.portal-return {
  margin-top: auto;
}

.content {
  display: flex;
  flex-direction: column;
  min-height: 0;
  min-width: 0;
  height: 100%;
  overflow-x: hidden;
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  padding: 16px 20px 24px;
  box-sizing: border-box;
  background: #f6faf9;
}

.page-viewport {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 0;
  overflow: visible;
}

:deep(.page) {
  flex: 1;
  width: 100%;
  min-height: 100%;
  box-sizing: border-box;
  align-content: start;
}

.topbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
  gap: 12px;
  padding: 2px 0 12px;
  border-bottom: 1px solid #dfe7df;
}

.eyebrow {
  margin: 0;
  color: #6d7b72;
  font-size: 12px;
  font-weight: 700;
}

.topbar-actions {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

button {
  border: 1px solid #dfe7df;
  border-radius: 8px;
  padding: 10px 14px;
  background: #ffffff;
  color: #245849;
  font-weight: 700;
  cursor: pointer;
  transition: background-color 160ms ease, border-color 160ms ease;
}

button:hover {
  border-color: rgba(36, 88, 73, 0.32);
  background: #f4f7f4;
}

@media (max-width: 840px) {
  .warehouse-stage {
    padding: 0;
    overflow: auto;
  }

  .shell {
    grid-template-columns: 1fr;
    min-height: 100vh;
    height: auto;
    border-radius: 0;
  }

  .sidebar {
    position: static;
    height: auto;
    border-right: 0;
    border-bottom: 1px solid #dfe7df;
  }

  nav {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .content {
    height: auto;
    padding: 14px;
  }
}
</style>
