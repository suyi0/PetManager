<template>
  <div class="saas-shell">
    <aside class="saas-sidebar">
      <div class="saas-brand">
        <div class="saas-brand__logo">宠</div>
        <div>
          <div class="saas-brand__name">宠物医院</div>
          <div class="saas-brand__sub">管理端</div>
        </div>
      </div>

      <nav class="saas-nav">
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/overview`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <rect x="3" y="3" width="7" height="9" rx="1" />
            <rect x="14" y="3" width="7" height="5" rx="1" />
            <rect x="14" y="12" width="7" height="9" rx="1" />
            <rect x="3" y="16" width="7" height="5" rx="1" />
          </svg>
          总览
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/worktime`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <circle cx="12" cy="12" r="9" />
            <path d="M12 7v5l3 2" />
          </svg>
          考勤管理
        </RouterLink>
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/users`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2" />
            <circle cx="9" cy="7" r="4" />
            <path d="M22 21v-2a4 4 0 0 0-3-3.87" />
            <path d="M16 3.13a4 4 0 0 1 0 7.75" />
          </svg>
          用户管理
        </RouterLink>
        <RouterLink
          class="saas-nav__item"
          :to="`${routePrefix}/online-doctors`"
        >
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path
              d="M19 14c1.5-1.5 3-3.2 3-5.5A3.5 3.5 0 0 0 12 6 3.5 3.5 0 0 0 2 8.5c0 2.3 1.5 4 3 5.5l7 7Z"
            />
          </svg>
          在线医生
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
          日志审计
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
        返回总裁端
      </button>

      <PortalAccount
        :fallback-name="currentRoleLabel"
        profile-to="/super-admin/personal"
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
import { isSuperAdminPortalRole } from "@/core/auth/utils/roleUtils";
import {
  startSuperAdminSessionGuard,
  stopSuperAdminSessionGuard,
} from "@/modules/super-admin/utils/superAdminSessionGuard";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";
import PortalAccount from "@/shared/components/PortalAccount.vue";

export default defineComponent({
  name: "SuperAdminLayout",
  components: { PortalAccount },
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
/* 顶栏已移除：内容区单行，路由页直接撑满视口、不滚动。 */
.saas-content {
  grid-template-rows: minmax(0, 1fr);
}

.saas-content :deep(.page) {
  min-height: 0;
  height: 100%;
}
</style>
