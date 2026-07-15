<template>
  <div class="shell">
    <aside class="sidebar">
      <div class="brand">
        <div class="brand__logo">宠</div>
        <div>
          <div class="brand__name">宠物医院</div>
          <div class="brand__sub">人事端</div>
        </div>
      </div>

      <nav class="nav">
        <RouterLink class="nav__item" :to="`${routePrefix}/access`">
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
          任职与薪酬
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
        profile-to="/personnel/personal"
        @logout="logout"
      />
    </aside>

    <main class="content">
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
  startPersonnelSessionGuard,
  stopPersonnelSessionGuard,
} from "@/modules/personnel/utils/personnelSessionGuard";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";
import PortalAccount from "@/shared/components/PortalAccount.vue";

export default defineComponent({
  name: "PersonnelLayout",
  components: { PortalAccount },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const { showBossReturn, returnLabel, returnToBossPortal } =
      useBossPortalReturn(router);

    onMounted(() => {
      startPersonnelSessionGuard(store, router);
    });

    onBeforeUnmount(() => {
      stopPersonnelSessionGuard();
    });

    const logout = () => {
      stopPersonnelSessionGuard();
      void store.dispatch("auth/logout");
    };

    const routePrefix = computed(() => "/personnel");

    const currentRoleLabel = computed(() => {
      // 展示当前职位名（动态职位原样显示）；能进本布局已由路由守卫按权限保证
      return store.state.auth.userRole || "人事经理";
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
.shell {
  --indigo: #4f46e5;
  --indigo-50: #eef2ff;
  --text: #0f172a;
  --muted: #64748b;
  --faint: #94a3b8;
  --border: #e7e9ee;

  display: grid;
  grid-template-columns: 248px 1fr;
  height: 100vh;
  overflow: hidden;
  background: #f6f7f9;
  color: var(--text);
  font-family: "PingFang SC", "Segoe UI", system-ui, sans-serif;
}

.sidebar {
  display: flex;
  flex-direction: column;
  padding: 20px 16px;
  background: #ffffff;
  border-right: 1px solid var(--border);
}

.brand {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 6px 8px 18px;
}

.brand__logo {
  width: 34px;
  height: 34px;
  border-radius: 9px;
  background: linear-gradient(135deg, #6366f1, #4f46e5);
  color: #fff;
  display: grid;
  place-items: center;
  font-weight: 800;
  font-size: 16px;
}

.brand__name {
  font-size: 15px;
  font-weight: 700;
}

.brand__sub {
  font-size: 12px;
  color: var(--faint);
}

.nav {
  display: grid;
  gap: 4px;
  margin-top: 6px;
}

.nav__item {
  display: flex;
  align-items: center;
  gap: 10px;
  height: 40px;
  padding: 0 12px;
  border-radius: 9px;
  color: var(--muted);
  font-size: 14px;
  font-weight: 600;
  text-decoration: none;
  cursor: pointer;
}

.nav__item:hover {
  background: #f1f2f5;
  color: var(--text);
}

.nav__item.router-link-active {
  background: var(--indigo-50);
  color: var(--indigo);
}

.nav__item svg {
  width: 18px;
  height: 18px;
}

.sidebar__foot {
  margin-top: auto;
  display: grid;
  gap: 10px;
}

.role-card {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px 12px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: #fafbfc;
}

.role-card__avatar {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  background: var(--indigo-50);
  color: var(--indigo);
  display: grid;
  place-items: center;
  font-weight: 700;
  font-size: 13px;
}

.role-card__name {
  font-size: 13px;
  font-weight: 700;
}

.role-card__role {
  font-size: 12px;
  color: var(--muted);
}

.btn-return {
  height: 38px;
  border: 1px solid var(--border);
  border-radius: 9px;
  background: #fff;
  color: var(--muted);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.btn-return:hover {
  border-color: var(--indigo);
  color: var(--indigo);
}

.content {
  display: grid;
  grid-template-rows: minmax(0, 1fr);
  padding: 18px 24px 22px;
  height: 100vh;
  min-height: 0;
  box-sizing: border-box;
  overflow: hidden;
}

.topbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.topbar__title {
  margin: 0;
  font-size: 20px;
  font-weight: 700;
  letter-spacing: -0.01em;
  color: var(--text);
}

.topbar__sub {
  font-size: 13px;
  color: var(--muted);
  margin-top: 2px;
}

.btn-logout {
  height: 38px;
  padding: 0 16px;
  border: 1px solid var(--border);
  border-radius: 9px;
  background: #fff;
  color: var(--text);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.btn-logout:hover {
  background: #f6f7f9;
}

@media (max-width: 1100px) {
  .shell {
    grid-template-columns: 1fr;
    height: auto;
    overflow: visible;
  }

  .sidebar {
    height: auto;
  }

  .content {
    height: auto;
    overflow: visible;
    grid-template-rows: auto auto;
  }
}
</style>
