<template>
  <div class="saas-shell">
    <aside class="saas-sidebar">
      <div class="saas-brand">
        <div class="saas-brand__logo">宠</div>
        <div>
          <div class="saas-brand__name">宠物医院</div>
          <div class="saas-brand__sub">财务端</div>
        </div>
      </div>

      <nav class="saas-nav">
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/salary`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <circle cx="12" cy="12" r="9" />
            <path
              d="M14.5 9a2.5 2.5 0 0 0-2.5-1.5c-1.4 0-2.5.8-2.5 2s1 1.7 2.5 2 2.5.9 2.5 2-1.1 2-2.5 2A2.5 2.5 0 0 1 9.5 15"
            />
            <path d="M12 6v1.5M12 16.5V18" />
          </svg>
          工资管理
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

      <PortalAccount
        :fallback-name="currentRoleLabel"
        profile-to="/finance/personal"
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
import {
  startFinanceSessionGuard,
  stopFinanceSessionGuard,
} from "@/modules/finance/utils/financeSessionGuard";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";
import PortalAccount from "@/shared/components/PortalAccount.vue";

export default defineComponent({
  name: "FinanceLayout",
  components: { PortalAccount },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const { showBossReturn, returnLabel, returnToBossPortal } = useBossPortalReturn(router);

    onMounted(() => {
      startFinanceSessionGuard(store, router);
      void store.dispatch("finance/ensureHomeData");
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
      // 展示当前职位名（动态职位原样显示）；能进本布局已由路由守卫按权限保证
      return store.state.auth.userRole || "财务总监";
    });

    return {
      logout,
      routePrefix,
      currentRoleLabel,
      showBossReturn,
      returnLabel,
      returnToBossPortal,
    };
  },
});
</script>

<style scoped>
/* 外壳来自全局 saas.css；顶栏已移除，内容区作自然滚动列。 */
.saas-content {
  display: block;
  overflow-y: auto;
}
</style>
