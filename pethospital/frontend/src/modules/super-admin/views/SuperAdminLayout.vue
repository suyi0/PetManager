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
        <RouterLink
          v-if="canReadAttendance"
          class="saas-nav__item"
          :to="`${routePrefix}/worktime`"
        >
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
        <RouterLink class="saas-nav__item" :to="`${routePrefix}/rbac`">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="M12 3 4 7v5c0 5 3.4 8 8 9 4.6-1 8-4 8-9V7l-8-4Z" />
            <path d="M9 12h6" />
            <path d="M12 9v6" />
          </svg>
          权限组织
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
        <RouterLink
          v-if="canReadTemplates"
          class="saas-nav__item"
          :to="`${routePrefix}/report-templates`"
        >
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="M6 3h9l3 3v15H6z" />
            <path d="M9 11h6M9 15h6" />
          </svg>
          文书模板
        </RouterLink>
      </nav>

      <section v-if="portalEntries.length" class="switcher" aria-label="巡检入口">
        <p class="switcher__title">巡检入口 · 以当前账号查看他端</p>
        <button
          v-for="portal in portalEntries"
          :key="portal.key"
          type="button"
          class="switcher__btn"
          :title="portal.hint"
          @click="enterPortal(portal)"
        >
          <span>{{ portal.label }}</span>
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <path d="m9 6 6 6-6 6" />
          </svg>
        </button>
      </section>

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
import { useRoute, useRouter } from "vue-router";
import {
  startSuperAdminSessionGuard,
  stopSuperAdminSessionGuard,
} from "@/modules/super-admin/utils/superAdminSessionGuard";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";
import { authStorage } from "@/core/auth/utils/authStorage";
import PortalAccount from "@/shared/components/PortalAccount.vue";

// 巡检入口清单：超管以当前账号跳入各业务端查功能。每项按对应 portal 权限过滤，
// 未授权（如尚未重新登录拿到新权限）则不显示，避免死链。不含总裁端（会抢落地页）。
interface PortalEntry {
  key: string;
  label: string;
  hint: string;
  perm: string;
  path: string;
}
const INSPECTION_PORTALS: PortalEntry[] = [
  { key: "doctor", label: "医生端", hint: "工作台 / 候诊队列 / 开单", perm: "portal:medical", path: "/doctor/home" },
  { key: "personnel", label: "人事端", hint: "职位任职 / 薪资管理", perm: "portal:personnel", path: "/personnel/employment" },
  { key: "warehouse", label: "仓库端", hint: "库存 / 预警 / 出入库", perm: "portal:warehouse", path: "/warehouse-admin/dashboard" },
  { key: "finance", label: "财务端", hint: "薪资 / 财务报表", perm: "portal:finance", path: "/finance/salary" },
  { key: "user", label: "用户端", hint: "首页 / 服务预约 / 个人资料", perm: "portal:user", path: "/user/home" },
];

export default defineComponent({
  name: "SuperAdminLayout",
  components: { PortalAccount },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const route = useRoute();
    const { showBossReturn, returnLabel, returnToBossPortal } = useBossPortalReturn(router);

    // 只列出当前账号确有 portal 权限的端（权限来自登录快照，授权后需重新登录才出现）。
    const portalEntries = computed(() =>
      INSPECTION_PORTALS.filter((entry) =>
        store.state.auth.permissions.includes(entry.perm)
      )
    );

    // 跳入他端：复用总裁端已验证的做法——存返回标记（回到当前超管页）后 router.push。
    // 不换 token，靠已授予的 portal 权限放行；目标端显示「返回管理员端」。
    const enterPortal = async (portal: PortalEntry) => {
      authStorage.saveBossPortalReturn({
        returnTo: route.fullPath || "/super-admin/overview",
      });
      await router.push(portal.path);
    };

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
    const canReadAttendance = computed(() =>
      store.state.auth.permissions.includes("attendance:read")
    );
    const canReadTemplates = computed(() =>
      store.state.auth.permissions.includes("report-template:read")
    );

    const currentRoleLabel = computed(() => {
      // 展示当前职位名（动态职位原样显示）；能进本布局已由路由守卫按权限保证
      return store.state.auth.userRole || "超级管理员";
    });

    return {
      logout,
      routePrefix,
      canReadAttendance,
      canReadTemplates,
      currentRoleLabel,
      portalEntries,
      enterPortal,
      showBossReturn,
      returnLabel,
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

/* 巡检入口：侧栏内的紧凑端口切换区（主色/描边取自 saas.css 晴空蓝 token） */
.switcher {
  display: grid;
  gap: 4px;
  margin-top: 16px;
}

.switcher__title {
  margin: 0 0 5px;
  padding: 0 4px;
  color: var(--saas-faint);
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.04em;
}

.switcher__btn {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  width: 100%;
  height: 36px;
  padding: 0 11px;
  border: 1px solid var(--saas-border);
  border-radius: 9px;
  background: var(--saas-surface);
  color: var(--saas-muted);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.15s ease, border-color 0.15s ease, color 0.15s ease;
}

.switcher__btn:hover {
  background: var(--saas-indigo-50);
  border-color: var(--saas-indigo);
  color: var(--saas-indigo);
}

.switcher__btn:focus-visible {
  outline: 2px solid var(--saas-indigo);
  outline-offset: 2px;
}

.switcher__btn svg {
  width: 15px;
  height: 15px;
  opacity: 0.55;
}
</style>
