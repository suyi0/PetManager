<template>
  <div class="warehouse-stage">
    <div class="shell">
      <aside class="sidebar">
        <div class="brand-stack">
          <div class="logo-core">PM</div>
          <div class="brand-copy"></div>
        </div>

        <nav>
          <RouterLink :to="`${routePrefix}/dashboard`">仪表盘</RouterLink>
          <RouterLink :to="`${routePrefix}/create`">入库</RouterLink>
          <RouterLink :to="`${routePrefix}/warnings`">紧急</RouterLink>
          <RouterLink :to="`${routePrefix}/logs`">日志</RouterLink>
        </nav>

        <button
          v-if="showBossReturn"
          type="button"
          class="boss-return"
          @click="returnToBossPortal"
        >
          返回总裁端
        </button>

        <button
          v-if="showAdminReturn"
          type="button"
          class="admin-return"
          @click="returnToSuperAdmin"
        >
          返回超级管理员
        </button>
      </aside>

      <main class="content">
        <header class="topbar">
          <div>
            <p class="eyebrow">PetManager Warehouse</p>
          </div>
          <div class="topbar-actions">
            <button @click="logout">登出</button>
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
      // 仓库管理员进入任意页面时，先预热仪表盘依赖的基础缓存。
      void store.dispatch("warehouseAdmin/ensureDashboardData");
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
  padding: clamp(12px, 2vw, 20px);
  overflow: hidden;
  background: radial-gradient(
      circle at 8% 16%,
      rgba(95, 191, 255, 0.34),
      transparent 20%
    ),
    radial-gradient(
      circle at 88% 10%,
      rgba(74, 146, 255, 0.22),
      transparent 26%
    ),
    linear-gradient(180deg, #cddff7 0%, #d8e8fb 52%, #cfdef5 100%);
}

.shell {
  position: relative;
  display: grid;
  grid-template-columns: minmax(112px, 14vw) minmax(0, 1fr);
  height: calc(100vh - clamp(24px, 4vw, 40px));
  min-height: 0;
  width: min(100%, 1536px);
  margin: 0 auto;
  overflow: clip;
  border-radius: clamp(16px, 2vw, 18px);
  border: 1px solid rgba(132, 192, 255, 0.28);
  background: linear-gradient(
      180deg,
      rgba(225, 236, 252, 0.96),
      rgba(204, 222, 248, 0.98)
    ),
    linear-gradient(180deg, #ddeafb, #c8daf4);
  box-shadow: 0 28px 72px rgba(61, 117, 194, 0.22),
    inset 0 1px 0 rgba(255, 255, 255, 0.45);
  color: #183d66;
  font-family: "Noto Sans SC", "Segoe UI", sans-serif;
}

.shell::before {
  content: "";
  position: absolute;
  inset: 76px 0 0 clamp(112px, 14vw, 220px);
  border-top: 1px solid rgba(142, 196, 255, 0.32);
  pointer-events: none;
}

.sidebar {
  position: sticky;
  top: 0;
  display: flex;
  flex-direction: column;
  gap: 28px;
  min-width: 0;
  height: 100%;
  min-height: 0;
  padding: clamp(18px, 2vw, 20px) clamp(14px, 1.6vw, 16px) 18px;
  border-right: 1px solid rgba(141, 192, 255, 0.28);
  background: radial-gradient(
      circle at top,
      rgba(110, 195, 255, 0.22),
      transparent 34%
    ),
    linear-gradient(
      180deg,
      rgba(215, 228, 249, 0.98),
      rgba(196, 214, 242, 0.96)
    );
}

.brand-stack {
  display: grid;
  gap: 18px;
}

.logo-core {
  display: grid;
  place-items: center;
  width: 58px;
  height: 58px;
  margin: 0 auto;
  border-radius: 18px;
  border: 1px solid rgba(117, 196, 255, 0.36);
  background: radial-gradient(
      circle at 50% 30%,
      rgba(146, 222, 255, 0.54),
      transparent 64%
    ),
    linear-gradient(180deg, rgba(98, 180, 255, 0.92), rgba(67, 124, 255, 0.96));
  color: #ffffff;
  font-weight: 800;
  letter-spacing: 0.08em;
  box-shadow: 0 0 0 6px rgba(142, 206, 255, 0.22),
    0 0 24px rgba(89, 171, 255, 0.2);
}

.eyebrow {
  margin: 0 0 6px;
  color: #3e73d6;
  text-transform: uppercase;
  letter-spacing: 0.12em;
  font-size: 11px;
}

.sidebar h1,
.topbar h2 {
  margin: 0;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
}

.sidebar h1 {
  font-size: 18px;
  font-weight: 700;
}

nav {
  display: grid;
  gap: 10px;
}

.sidebar a {
  padding: 12px 14px;
  border-radius: 12px;
  color: #456187;
  text-decoration: none;
  font-size: 12px;
  border: 1px solid rgba(161, 204, 255, 0.26);
  background: linear-gradient(
    180deg,
    rgba(226, 237, 251, 0.92),
    rgba(194, 214, 242, 0.94)
  );
}

.sidebar a.router-link-active {
  color: #0f3966;
  border-color: rgba(92, 163, 255, 0.34);
  background: linear-gradient(
    180deg,
    rgba(173, 207, 255, 0.98),
    rgba(148, 189, 247, 0.96)
  );
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.35),
    0 12px 20px rgba(85, 132, 205, 0.2);
}

.admin-return {
  margin-top: auto;
  border: 1px solid rgba(114, 178, 255, 0.34);
  border-radius: 14px;
  padding: 12px 14px;
  background: linear-gradient(180deg, #eef7ff, #dbeaff);
  color: #0f3966;
  font-size: 12px;
  font-weight: 800;
  cursor: pointer;
  box-shadow: 0 12px 24px rgba(90, 149, 228, 0.16);
}

.boss-return {
  margin-top: auto;
  border: 1px solid rgba(114, 178, 255, 0.34);
  border-radius: 14px;
  padding: 12px 14px;
  background: linear-gradient(180deg, #fff6e7, #dfefff);
  color: #0f3966;
  font-size: 12px;
  font-weight: 800;
  cursor: pointer;
  text-align: left;
  box-shadow: 0 12px 24px rgba(90, 149, 228, 0.16);
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
  padding: clamp(16px, 2vw, 18px) clamp(16px, 2vw, 22px) clamp(22px, 3vw, 28px)
    clamp(12px, 1.6vw, 16px);
  box-sizing: border-box;
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
  margin-bottom: 18px;
  gap: 12px;
  padding: 8px 12px 16px;
}

.topbar h2 {
  font-size: 18px;
  font-weight: 700;
}

.topbar-actions {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
  border-radius: 999px;
  border: 1px solid rgba(157, 205, 255, 0.32);
  background: rgba(224, 236, 252, 0.74);
}

.topbar-actions span {
  font-size: 12px;
  color: #4d6790;
}

button {
  border: 1px solid rgba(106, 175, 255, 0.24);
  border-radius: 999px;
  padding: 9px 14px;
  background: linear-gradient(135deg, #7dd8ff, #5a9dff);
  color: #062448;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 10px 24px rgba(87, 160, 255, 0.22);
}

@media (max-width: 1080px) {
  .warehouse-stage {
    padding: 12px;
  }

  .shell {
    grid-template-columns: 1fr;
    height: calc(100vh - 24px);
    min-height: 0;
  }

  .shell::before {
    inset: 76px 0 0 0;
  }

  .sidebar {
    position: relative;
    top: auto;
    gap: 20px;
    height: auto;
    min-height: auto;
    max-height: 40vh;
    overflow-y: auto;
    border-right: 0;
    border-bottom: 1px solid rgba(116, 183, 255, 0.12);
  }

  nav {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .topbar {
    flex-direction: column;
    align-items: flex-start;
    gap: 12px;
  }

  .topbar-actions {
    flex-wrap: wrap;
  }

  .content {
    height: 100%;
    min-height: 0;
    overflow-y: auto;
  }
}

@media (max-width: 720px) {
  .warehouse-stage {
    padding: 0;
  }

  .shell {
    height: 100vh;
    min-height: 0;
    border-radius: 0;
  }

  .sidebar {
    padding: 16px 14px;
  }

  nav {
    grid-template-columns: 1fr;
  }

  .content {
    padding: 14px;
  }

  .topbar {
    padding: 4px 4px 14px;
  }

  .topbar-actions {
    width: 100%;
    border-radius: 18px;
  }

  .topbar-actions span,
  button {
    width: 100%;
    text-align: center;
  }
}
</style>
