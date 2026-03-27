<template>
  <div class="user-shell">
    <div class="user-shell__glow user-shell__glow--left"></div>
    <div class="user-shell__glow user-shell__glow--right"></div>

    <header class="user-topbar">
      <div class="brand-block">
        <p class="brand-block__eyebrow">Pet Wellness Center</p>
        <div>
          <h1>PetManager 用户端</h1>
          <span>预约、档案、订单整合在一个轻松易读的空间里。</span>
        </div>
      </div>

      <nav class="topbar-nav">
        <RouterLink
          v-for="item in navItems"
          :key="item.key"
          :to="item.to"
          class="topbar-nav__link"
          :class="{ 'topbar-nav__link--active': isRouteActive(item.key) }"
        >
          {{ item.label }}
        </RouterLink>
      </nav>

      <div class="profile-panel" ref="personalMenu">
        <button class="profile-trigger" @click.stop="togglePersonalMenu">
          <div v-if="userAvatar" class="profile-trigger__avatar">
            <img :src="userAvatar" alt="用户头像" />
          </div>
          <div
            v-else
            class="profile-trigger__avatar profile-trigger__avatar--fallback"
          >
            {{ userInitial }}
          </div>

          <div class="profile-trigger__copy">
            <strong>{{ userDisplayName }}</strong>
            <span>{{ userSubtitle }}</span>
          </div>

          <span class="profile-trigger__caret">⌄</span>
        </button>

        <transition name="menu-fade">
          <div v-if="personal" class="profile-menu">
            <div class="profile-menu__hero">
              <div
                class="profile-menu__badge"
                :class="{ 'profile-menu__badge--fallback': !userAvatar }"
              >
                <img v-if="userAvatar" :src="userAvatar" alt="用户头像" />
                <span v-else>{{ userInitial }}</span>
              </div>
              <div>
                <strong>{{ userDisplayName }}</strong>
                <span>{{ userSubtitle }}</span>
              </div>
            </div>

            <div class="profile-menu__actions">
              <button
                v-for="item in quickActions"
                :key="item.label"
                class="profile-menu__action"
                @click="goTo(item.to)"
              >
                <span>{{ item.label }}</span>
                <small>{{ item.hint }}</small>
              </button>
            </div>

            <div class="profile-menu__footer">
              <button class="profile-menu__ghost" @click="logout">
                切换账号
              </button>
              <button class="profile-menu__danger" @click="logout">
                退出登录
              </button>
            </div>
          </div>
        </transition>
      </div>
    </header>

    <main class="user-stage">
      <RouterView />
    </main>

    <button
      v-if="showAdminReturn"
      type="button"
      class="admin-return-fab"
      @click="returnToSuperAdmin"
    >
      返回超级管理员
    </button>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";
import { useRoute, useRouter } from "vue-router";
import { authStorage } from "@/core/auth/utils/authStorage";

const store = useStore(storeKey);
const route = useRoute();
const router = useRouter();

const basePath = computed(() =>
  route.path.startsWith("/preview/user") ? "/preview/user" : "/user"
);

const navItems = computed(() => [
  { key: "home", label: "首页", to: `${basePath.value}/home` },
  { key: "services", label: "服务预约", to: `${basePath.value}/services` },
  { key: "order", label: "订单记录", to: `${basePath.value}/order` },
]);

const quickActions = computed(() => [
  {
    label: "个人中心",
    hint: "查看资料与地址",
    to: `${basePath.value}/personal`,
  },
  {
    label: "宠物档案",
    hint: "维护宠物资料与护理备注",
    to: `${basePath.value}/personal?tab=pet`,
  },
  { label: "我的订单", hint: "订单与预约记录", to: `${basePath.value}/order` },
  {
    label: "账号安全",
    hint: "返回个人资料页维护信息",
    to: `${basePath.value}/personal?tab=personal`,
  },
]);

const personal = computed(() => store.state.ui.personal);
const userAvatar = computed(() => store.state.currentUser.userHeadImage || "");
const userDisplayName = computed(
  () => store.getters["auth/formattedUserName"] || "体验用户"
);
const userSubtitle = computed(
  () => store.state.currentUser.userEmail || "未绑定邮箱"
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
  if (store.state.ui.personal) {
    store.commit("ui/closePersonal");
    return;
  }
  store.commit("ui/openPersonal");
};

const closePersonal = () => {
  if (store.state.ui.personal) {
    store.commit("ui/closePersonal");
  }
};

const goTo = (to: string) => {
  closePersonal();
  router.push(to);
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
  store.commit("auth/setSession", {
    token: bridge.token,
    userType: bridge.userType,
    userRole: bridge.userRole,
  });
  store.commit(
    "currentUser/setCurrentUser",
    {
      userType: bridge.userType,
      userRole: bridge.userRole,
      userName: bridge.userName,
      userPhone: bridge.userPhone,
      userEmail: bridge.userEmail,
      userBirthday: bridge.userBirthday,
      userAddressId: bridge.userAddressId,
      userAddress: bridge.userAddress || "",
      userHeadImage: bridge.userHeadImage || "",
    },
    { root: true }
  );
  authStorage.clearAdminPortalBridge();
  await router.push(bridge.returnTo);
};

const isRouteActive = (key: string) => route.path.includes(`/${key}`);

const handleClickOutside = (event: MouseEvent) => {
  const menu = document.querySelector(".profile-panel");
  if (menu && !menu.contains(event.target as Node)) {
    closePersonal();
  }
};

onMounted(() => {
  document.addEventListener("click", handleClickOutside);
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
  --shell-bg: #f7f4ea;
  --shell-panel: rgba(255, 250, 239, 0.92);
  --shell-border: rgba(22, 90, 94, 0.12);
  --shell-shadow: 0 30px 70px rgba(29, 93, 95, 0.12);
  --shell-text: #163f42;
  --shell-muted: #5f7d7a;
  --shell-accent: #1d8687;
  --shell-accent-strong: #0f6868;
  --shell-warm: #e38b63;
  position: relative;
  min-height: 100vh;
  padding: 24px;
  background: radial-gradient(
      circle at 10% 10%,
      rgba(126, 210, 205, 0.28),
      transparent 20%
    ),
    radial-gradient(
      circle at 85% 12%,
      rgba(243, 196, 152, 0.28),
      transparent 22%
    ),
    linear-gradient(180deg, #f4efe1 0%, #eef6f3 48%, #f3efe6 100%);
  color: var(--shell-text);
  font-family: "Noto Sans SC", "PingFang SC", "Segoe UI", sans-serif;
}

.user-shell__glow {
  position: fixed;
  width: 320px;
  height: 320px;
  border-radius: 50%;
  filter: blur(10px);
  pointer-events: none;
}

.user-shell__glow--left {
  left: -120px;
  bottom: 120px;
  background: rgba(117, 201, 190, 0.2);
}

.user-shell__glow--right {
  right: -120px;
  top: 84px;
  background: rgba(241, 179, 144, 0.16);
}

.admin-return-fab {
  position: fixed;
  left: 24px;
  bottom: 24px;
  z-index: 30;
  border: 1px solid rgba(29, 134, 135, 0.18);
  border-radius: 999px;
  padding: 12px 18px;
  background: linear-gradient(135deg, #fff9ef, #e3f6f0);
  color: #17494d;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 16px 34px rgba(22, 90, 94, 0.16);
}

.user-topbar {
  position: sticky;
  top: 16px;
  z-index: 20;
  display: grid;
  grid-template-columns: minmax(240px, 1.1fr) auto minmax(240px, 320px);
  align-items: center;
  gap: 18px;
  width: min(1320px, 100%);
  margin: 0 auto 22px;
  padding: 18px 22px;
  border: 1px solid var(--shell-border);
  border-radius: 28px;
  background: rgba(255, 249, 238, 0.78);
  backdrop-filter: blur(22px);
  box-shadow: var(--shell-shadow);
}

.brand-block {
  display: grid;
  gap: 4px;
}

.brand-block__eyebrow {
  margin: 0;
  color: var(--shell-accent);
  letter-spacing: 0.12em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.brand-block h1 {
  margin: 0;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: clamp(28px, 3vw, 34px);
  line-height: 1;
}

.brand-block span {
  color: var(--shell-muted);
  font-size: 13px;
}

.topbar-nav {
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  gap: 10px;
}

.topbar-nav__link {
  padding: 10px 16px;
  border-radius: 999px;
  border: 1px solid rgba(29, 134, 135, 0.12);
  color: var(--shell-text);
  text-decoration: none;
  font-size: 14px;
  font-weight: 600;
  background: rgba(255, 255, 255, 0.56);
  transition: transform 0.2s ease, box-shadow 0.2s ease, background 0.2s ease;
}

.topbar-nav__link:hover,
.topbar-nav__link--active {
  transform: translateY(-1px);
  background: linear-gradient(
    135deg,
    rgba(132, 214, 205, 0.28),
    rgba(240, 195, 158, 0.24)
  );
  box-shadow: 0 12px 28px rgba(24, 95, 97, 0.1);
}

.profile-panel {
  position: relative;
}

.profile-trigger {
  width: 100%;
  display: grid;
  grid-template-columns: 52px 1fr 16px;
  align-items: center;
  gap: 12px;
  padding: 10px 12px;
  border: 1px solid rgba(29, 134, 135, 0.12);
  border-radius: 22px;
  background: rgba(255, 255, 255, 0.66);
  color: inherit;
  cursor: pointer;
}

.profile-trigger__avatar,
.profile-menu__badge {
  width: 52px;
  height: 52px;
  border-radius: 18px;
  overflow: hidden;
  display: grid;
  place-items: center;
  background: linear-gradient(135deg, #95ddd4, #f0c59c);
  color: #114a4a;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 24px;
  font-weight: 700;
}

.profile-trigger__avatar img,
.profile-menu__badge img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.profile-trigger__avatar--fallback,
.profile-menu__badge--fallback {
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.4);
}

.profile-trigger__copy {
  display: grid;
  justify-items: start;
}

.profile-trigger__copy strong,
.profile-menu__hero strong {
  font-size: 15px;
}

.profile-trigger__copy span,
.profile-menu__hero span {
  color: var(--shell-muted);
  font-size: 12px;
}

.profile-trigger__caret {
  color: var(--shell-accent);
  font-size: 18px;
}

.profile-menu {
  position: absolute;
  right: 0;
  top: calc(100% + 10px);
  width: min(320px, 92vw);
  padding: 16px;
  border-radius: 26px;
  border: 1px solid rgba(29, 134, 135, 0.12);
  background: var(--shell-panel);
  box-shadow: 0 22px 48px rgba(27, 91, 92, 0.16);
}

.profile-menu__hero {
  display: grid;
  grid-template-columns: 52px 1fr;
  gap: 12px;
  align-items: center;
  margin-bottom: 14px;
}

.profile-menu__actions {
  display: grid;
  gap: 10px;
}

.profile-menu__action {
  width: 100%;
  display: grid;
  justify-items: start;
  gap: 2px;
  padding: 12px 14px;
  border: 1px solid rgba(29, 134, 135, 0.08);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.56);
  color: inherit;
  cursor: pointer;
}

.profile-menu__action span {
  font-size: 14px;
  font-weight: 700;
}

.profile-menu__action small {
  color: var(--shell-muted);
  font-size: 12px;
}

.profile-menu__footer {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-top: 14px;
}

.profile-menu__ghost,
.profile-menu__danger {
  padding: 12px 14px;
  border: none;
  border-radius: 16px;
  cursor: pointer;
  font-weight: 700;
}

.profile-menu__ghost {
  background: rgba(29, 134, 135, 0.08);
  color: var(--shell-accent-strong);
}

.profile-menu__danger {
  background: linear-gradient(135deg, #eb9f7f, #d87457);
  color: #fffaf5;
}

.user-stage {
  width: min(1320px, 100%);
  margin: 0 auto;
  min-height: 0;
}

.menu-fade-enter-active,
.menu-fade-leave-active {
  transition: opacity 0.18s ease, transform 0.18s ease;
}

.menu-fade-enter-from,
.menu-fade-leave-to {
  opacity: 0;
  transform: translateY(-6px);
}

@media (max-width: 1100px) {
  .user-topbar {
    grid-template-columns: 1fr;
  }

  .topbar-nav {
    justify-content: flex-start;
  }
}

@media (max-width: 720px) {
  .user-shell {
    padding: 14px;
  }

  .user-topbar {
    top: 10px;
    padding: 16px;
    border-radius: 22px;
  }

  .topbar-nav {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .topbar-nav__link {
    text-align: center;
  }
}
</style>
