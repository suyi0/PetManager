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
          ← 返回总裁端
        </button>

        <PortalAccount fallback-name="用户" @logout="logout" />
      </div>
    </aside>

    <main class="user-stage">
      <RouterView />
    </main>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { useRoute, useRouter } from "vue-router";
import { authStorage } from "@/core/auth/utils/authStorage";
import { useBossPortalReturn } from "@/core/auth/utils/bossPortalReturn";

const store = useStore(storeKey);
const route = useRoute();
const router = useRouter();
const { showBossReturn, returnToBossPortal } = useBossPortalReturn(router);

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
]);

const isActive = (item: { key: string; to: string }) =>
  route.path.startsWith(item.to.split("?")[0]);

const goTo = (to: string) => {
  closePersonal();
  void router.push(to);
};

const personal = computed(() => store.state.ui.personal);
const userAvatar = computed(() => store.state.currentUser.userHeadImage || "");
const userDisplayName = computed(
  () => store.getters["auth/formattedUserName"] || "体验用户"
);
const userInitial = computed(() =>
  String(userDisplayName.value || "U")
    .trim()
    .charAt(0)
    .toUpperCase()
);
const adminBridge = computed(() => authStorage.loadAdminPortalBridge());
const showAdminReturn = computed(() => Boolean(adminBridge.value));

const togglePersonalMenu = () => {
  void store.dispatch("ui/togglePersonal");
};

const closePersonal = () => {
  if (store.state.ui.personal) {
    void store.dispatch("ui/closePersonal");
  }
};

const logout = () => {
  closePersonal();
  store.dispatch("auth/logout");
};

const returnToSuperAdmin = async () => {
  const bridge = adminBridge.value;
  if (!bridge) {
    return;
  }

  closePersonal();
  await store.dispatch("auth/restoreAdminPortalBridgeSession", bridge);
  await router.push(bridge.returnTo);
};

/**
 * 监听全局点击事件以关闭个人菜单。
 * @param event 表示点击事件对象
 */
const handleClickOutside = (event: MouseEvent) => {
  const menu = document.querySelector(".profile-panel");
  if (menu && !menu.contains(event.target as Node)) {
    closePersonal();
  }
};

onMounted(() => {
  document.addEventListener("click", handleClickOutside);
  /**
   * 用户端首页进入后预热预约基础数据；订单/预约记录列表由对应页面进入时实时获取。
   */
  void store.dispatch("userPortal/ensureServiceData");
});

onBeforeUnmount(() => {
  document.removeEventListener("click", handleClickOutside);
});

watch(
  () => route.fullPath,
  () => {
    closePersonal();
  }
);
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

/* 用户卡 + 菜单（与其他端 PortalAccount 同款设计） */
.profile-panel {
  position: relative;
}

.acct-btn {
  width: 100%;
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 10px;
  border: 1px solid var(--shell-border);
  border-radius: 12px;
  background: #fffdfa;
  cursor: pointer;
  text-align: left;
}

.acct-btn:hover {
  background: #f3f7f4;
}

.acct-avatar {
  width: 34px;
  height: 34px;
  border-radius: 50%;
  flex: 0 0 auto;
  overflow: hidden;
  display: grid;
  place-items: center;
  background: #ffe8d2;
  color: var(--shell-warm);
  font-weight: 800;
  font-size: 14px;
}

.acct-avatar img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.acct-name {
  flex: 1;
  min-width: 0;
  font-size: 13px;
  font-weight: 700;
  color: var(--shell-text);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.acct-chev {
  width: 16px;
  height: 16px;
  color: var(--shell-faint);
  flex: 0 0 auto;
}

.acct-menu {
  position: absolute;
  left: 0;
  right: 0;
  bottom: calc(100% + 8px);
  padding: 6px;
  border-radius: 12px;
  border: 1px solid var(--shell-border);
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(27, 91, 92, 0.14);
  z-index: 20;
}

.acct-mi {
  width: 100%;
  display: flex;
  align-items: center;
  gap: 10px;
  height: 38px;
  padding: 0 10px;
  border: 0;
  border-radius: 8px;
  background: transparent;
  color: var(--shell-text);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  text-align: left;
}

.acct-mi:hover {
  background: #f3f7f4;
}

.acct-mi svg {
  width: 16px;
  height: 16px;
  color: var(--shell-muted);
}

.acct-divider {
  height: 1px;
  background: var(--shell-border);
  margin: 6px 4px;
}

.acct-mi--danger {
  color: #be4b5b;
}

.acct-mi--danger svg {
  color: #be4b5b;
}

.acct-mi--danger:hover {
  background: #fdeef0;
}

/* ===== 内容区 ===== */
.user-stage {
  min-width: 0;
  height: 100vh;
  overflow-y: auto;
  padding: 22px 26px 28px;
}

.menu-fade-enter-active,
.menu-fade-leave-active {
  transition: opacity 0.18s ease, transform 0.18s ease;
}

.menu-fade-enter-from,
.menu-fade-leave-to {
  opacity: 0;
  transform: translateY(6px);
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
