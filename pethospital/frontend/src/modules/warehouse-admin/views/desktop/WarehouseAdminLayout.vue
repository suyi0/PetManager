<template>
  <div class="saas-shell">
    <aside class="saas-sidebar">
      <div class="saas-brand">
        <div class="saas-brand__logo">宠</div>
        <div>
          <div class="saas-brand__name">宠物医院</div>
          <div class="saas-brand__sub">仓库端</div>
        </div>
      </div>

      <nav class="saas-nav">
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/dashboard`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="M3 9.5 12 4l9 5.5" />
            <path d="M5 10v9h14v-9" />
            <path d="M9 19v-5h6v5" />
          </svg>
          库存台账
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/create`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="M12 5v14" />
            <path d="M5 12h14" />
          </svg>
          新增入库
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/warnings`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path
              d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0Z"
            />
            <path d="M12 9v4" />
            <path d="M12 17h.01" />
          </svg>
          库存预警
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/logs`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="M8 6h11" />
            <path d="M8 12h11" />
            <path d="M8 18h11" />
            <path d="M3 6h.01" />
            <path d="M3 12h.01" />
            <path d="M3 18h.01" />
          </svg>
          日志
        </RouterLink>
      </nav>

      <button
        v-if="showBossReturn"
        type="button"
        class="saas-return"
        @click="returnToBossPortal"
      >
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
        >
          <path d="M19 12H5" />
          <path d="m12 19-7-7 7-7" />
        </svg>
        {{ returnLabel }}
      </button>
      <button
        v-if="showAdminReturn"
        type="button"
        class="saas-return"
        @click="returnToSuperAdmin"
      >
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
        >
          <path d="M19 12H5" />
          <path d="m12 19-7-7 7-7" />
        </svg>
        返回超级管理员
      </button>

      <PortalAccount
        fallback-name="仓库管理员"
        profile-to="/warehouse-admin/personal"
        @logout="logout"
      />
    </aside>

    <main class="saas-content">
      <RouterView />
    </main>
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
import PortalAccount from "@/shared/components/PortalAccount.vue";

export default defineComponent({
  name: "WarehouseAdminLayout",
  components: { PortalAccount },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const { showBossReturn, returnLabel, returnToBossPortal } = useBossPortalReturn(router);

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
      returnLabel,
      returnToBossPortal,
    };
  },
});
</script>

<style scoped>
/* 顶栏已移除；各页高度不一，内容区作自然滚动列。 */
.saas-content {
  display: block;
  overflow-y: auto;
}
</style>
