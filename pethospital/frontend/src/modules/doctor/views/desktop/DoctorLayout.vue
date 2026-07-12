<template>
  <div class="shell">
    <aside class="saas-sidebar">
      <div class="saas-brand">
        <div class="saas-brand__logo">宠</div>
        <div>
          <div class="saas-brand__name">宠物医院</div>
          <div class="saas-brand__sub">医生端</div>
        </div>
      </div>

      <nav class="saas-nav">
        <RouterLink class="saas-nav__item" :to="`${basePath}/home`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="M3 9.5 12 4l9 5.5" />
            <path d="M5 10v9h14v-9" />
          </svg>
          工作台
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${basePath}/queue`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <circle cx="12" cy="12" r="9" />
            <path d="M12 7v5l3 2" />
          </svg>
          待接诊队列
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${basePath}/reservations`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <rect x="3" y="4" width="18" height="17" rx="2" />
            <path d="M3 9h18" />
            <path d="M8 2v4" />
            <path d="M16 2v4" />
          </svg>
          预约订单
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${basePath}/order-records`">
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
          诊单记录
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${basePath}/drafts`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="M12 20h9" />
            <path d="M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4Z" />
          </svg>
          诊单草稿
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
        fallback-name="医生"
        profile-to="/doctor/personal"
        @logout="logout"
      />
    </aside>

    <main class="content">
      <RouterView />
    </main>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent, onBeforeUnmount, onMounted } from "vue";
import { useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { authStorage } from "@/core/auth/utils/authStorage";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";
import PortalAccount from "@/shared/components/PortalAccount.vue";
import { subscribeDoctorQueue } from "../../utils/doctorQueueStream";

export default defineComponent({
  name: "DoctorLayout",
  components: { PortalAccount },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const { showBossReturn, returnLabel, returnToBossPortal } = useBossPortalReturn(router);

    const basePath = computed(() => "/doctor");
    const adminBridge = computed(() => authStorage.loadAdminPortalBridge());
    const showAdminReturn = computed(() => Boolean(adminBridge.value));
    let closeQueueStream: (() => void) | null = null;

    onMounted(() => {
      // 医生端进入任意页面时，先预热工作台高频依赖的基础缓存。
      void store.dispatch("doctor/ensureWorkbenchData");
      closeQueueStream = subscribeDoctorQueue(
        (queueItems) => {
          store.commit("doctor/setQueueItems", queueItems);
        },
        {
          onFallbackRefresh: () => {
            void store.dispatch("doctor/ensureQueueItems", { force: true });
          },
        }
      );
    });

    onBeforeUnmount(() => {
      closeQueueStream?.();
      closeQueueStream = null;
    });

    const logout = () => {
      void store.dispatch("auth/logout");
    };

    const returnToSuperAdmin = async () => {
      const bridge = adminBridge.value;
      if (!bridge) {
        return;
      }

      await store.dispatch("auth/restoreAdminPortalBridgeSession", bridge);
      await router.push(bridge.returnTo);
    };

    return {
      basePath,
      logout,
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
/* 外壳保留医生页所需的 CSS 变量与栅格；侧栏改用全局 saas 样式，背景中性化。 */
.shell {
  --doctor-workspace-min-height: 760px;
  --doctor-page-card-height: min(calc(100vh - 60px), 860px);
  --doctor-table-row-height: 44px;
  --doctor-card-row-height: 128px;
  display: grid;
  width: 100%;
  grid-template-columns: 248px minmax(0, 1fr);
  height: 100vh;
  min-height: var(--doctor-workspace-min-height);
  align-items: stretch;
  overflow: hidden;
  background: #f6f7f9;
  color: #0f172a;
  font-family: "PingFang SC", "Noto Sans SC", sans-serif;
}

.content {
  min-width: 0;
  height: 100vh;
  min-height: 0;
  overflow: hidden;
  -webkit-overflow-scrolling: touch;
  padding: 26px 28px 34px;
  box-sizing: border-box;
}

@media (max-width: 980px) {
  .shell {
    --doctor-workspace-min-height: 0px;
    height: auto;
    grid-template-columns: 1fr;
    overflow: visible;
  }

  .content {
    height: auto;
    min-height: 0;
    overflow-y: visible;
    padding: 18px;
  }
}
</style>
