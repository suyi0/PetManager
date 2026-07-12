<template>
  <div class="user-shell">
    <aside class="user-side">
      <div class="side-brand">
        <div class="side-brand__logo">宠</div>
        <div>
          <b>宠物医院</b>
          <small>用户端</small>
        </div>
      </div>

      <nav class="side-nav">
        <RouterLink
          v-for="item in navItems"
          :key="item.key"
          :to="item.to"
          class="side-nav__link"
          :class="{ 'side-nav__link--active': isActive(item) }"
        >
          <span class="side-nav__ic">{{ item.icon }}</span>
          {{ item.label }}
        </RouterLink>
      </nav>

      <div class="side-foot">
        <button
          v-if="showAdminReturn"
          type="button"
          class="side-return"
          @click="returnToSuperAdmin"
        >
          ← 返回超级管理员
        </button>
        <button
          v-if="showBossReturn"
          type="button"
          class="side-return"
          @click="returnToBossPortal"
        >
          ← {{ returnLabel }}
        </button>

        <PortalAccount
          fallback-name="用户"
          profile-to="/user/personal"
          @logout="logout"
        />
      </div>
    </aside>

    <main class="user-stage">
      <RouterView />
    </main>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { useRoute, useRouter } from "vue-router";
import { authStorage } from "@/core/auth/utils/authStorage";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";
import PortalAccount from "@/shared/components/PortalAccount.vue";

const store = useStore(storeKey);
const route = useRoute();
const router = useRouter();
const { showBossReturn, returnLabel, returnToBossPortal } = useBossPortalReturn(router);

const basePath = computed(() => "/user");

const navItems = computed(() => [
  { key: "home", icon: "🏠", label: "首页", to: `${basePath.value}/home` },
  {
    key: "services",
    icon: "📅",
    label: "服务预约",
    to: `${basePath.value}/services`,
  },
  {
    key: "order",
    icon: "🧾",
    label: "我的订单",
    to: `${basePath.value}/order`,
  },
  {
    key: "attendance",
    icon: "⏱",
    label: "我的考勤",
    to: `${basePath.value}/attendance`,
  },
  {
    key: "medical-documents",
    icon: "📄",
    label: "诊疗文书",
    to: `${basePath.value}/medical-documents`,
  },
]);

const isActive = (item: { key: string; to: string }) =>
  route.path.startsWith(item.to.split("?")[0]);

const adminBridge = computed(() => authStorage.loadAdminPortalBridge());
const showAdminReturn = computed(() => Boolean(adminBridge.value));

const logout = () => {
  store.dispatch("auth/logout");
};

const returnToSuperAdmin = async () => {
  const bridge = adminBridge.value;
  if (!bridge) {
    return;
  }

  await store.dispatch("auth/restoreAdminPortalBridgeSession", bridge);
  await router.push(bridge.returnTo);
};

onMounted(() => {
  /**
   * 用户端首页进入后预热预约基础数据；订单/预约记录列表由对应页面进入时实时获取。
   */
  void store.dispatch("userPortal/ensureServiceData");
});
</script>

<style scoped lang="scss">
.user-shell {
  --shell-border: #efe7dc;
  --shell-text: #1f3a36;
  --shell-muted: #6b7d77;
  --shell-faint: #94a3b8;
  --shell-accent: #2f9e8f;
  --shell-accent-strong: #1f7a6c;
  --shell-accent-50: #e7f5f1;
  --shell-warm: #c2671b;

  display: grid;
  grid-template-columns: 232px minmax(0, 1fr);
  min-height: 100vh;
  background: linear-gradient(180deg, #fbfaf7 0%, #f1f7f4 100%);
  color: var(--shell-text);
  font-family: "Noto Sans SC", "PingFang SC", "Segoe UI", sans-serif;
}

/* ===== 侧边栏 ===== */
.user-side {
  position: sticky;
  top: 0;
  height: 100vh;
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 18px 14px;
  background: #ffffff;
  border-right: 1px solid var(--shell-border);
}

.side-brand {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 6px 8px 16px;
}

.side-brand__logo {
  width: 34px;
  height: 34px;
  border-radius: 10px;
  display: grid;
  place-items: center;
  font-weight: 800;
  color: #fff;
  background: linear-gradient(135deg, #2f9e8f, #38b2a3);
}

.side-brand b {
  font-size: 15px;
}

.side-brand small {
  display: block;
  font-size: 11px;
  color: var(--shell-faint);
}

.side-nav {
  display: grid;
  gap: 4px;
}

.side-nav__link {
  display: flex;
  align-items: center;
  gap: 10px;
  height: 42px;
  padding: 0 12px;
  border-radius: 11px;
  color: var(--shell-muted);
  font-size: 14px;
  font-weight: 600;
  text-decoration: none;
  transition: background 0.15s ease, color 0.15s ease;
}

.side-nav__ic {
  width: 20px;
  text-align: center;
  font-size: 16px;
}

.side-nav__link:hover {
  background: #f3f7f4;
  color: var(--shell-text);
}

.side-nav__link--active {
  background: var(--shell-accent-50);
  color: var(--shell-accent-strong);
}

.side-foot {
  margin-top: auto;
  display: grid;
  gap: 8px;
}

.side-return {
  height: 38px;
  border: 1px solid var(--shell-border);
  border-radius: 10px;
  background: #fffdfa;
  color: var(--shell-accent-strong);
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.side-return:hover {
  border-color: #cfe7e1;
}

/* ===== 内容区 ===== */
.user-stage {
  min-width: 0;
  height: 100vh;
  overflow-y: auto;
  padding: 22px 26px 28px;
}

@media (max-width: 860px) {
  .user-shell {
    grid-template-columns: 1fr;
  }

  .user-side {
    position: static;
    height: auto;
    flex-direction: row;
    flex-wrap: wrap;
    align-items: center;
  }

  .side-nav {
    grid-auto-flow: column;
  }

  .side-foot {
    margin-top: 0;
    margin-left: auto;
  }

  .user-stage {
    height: auto;
  }
}
</style>
