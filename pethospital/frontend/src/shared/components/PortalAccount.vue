<template>
  <div class="portal-account" @mouseleave="suppressed = false">
    <div
      class="portal-account__menu"
      :class="{ 'portal-account__menu--hidden': suppressed }"
      role="menu"
    >
      <button type="button" class="portal-account__mi" @click="goProfile">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
        >
          <circle cx="12" cy="8" r="4" />
          <path d="M4 21c0-4 4-6 8-6s8 2 8 6" />
        </svg>
        个人资料
      </button>
      <button type="button" class="portal-account__mi" @click="comingSoon">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
        >
          <circle cx="12" cy="12" r="3" />
          <path
            d="M19.4 13a1.7 1.7 0 0 0 .3 1.9l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.7 1.7 0 0 0-2.9 1.2V21a2 2 0 1 1-4 0v-.1A1.7 1.7 0 0 0 6 19.4l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1a1.7 1.7 0 0 0-1.2-2.9H2a2 2 0 1 1 0-4h.1A1.7 1.7 0 0 0 3.3 6l-.1-.1a2 2 0 1 1 2.8-2.8l.1.1a1.7 1.7 0 0 0 1.9.3H8a1.7 1.7 0 0 0 1-1.5V2a2 2 0 1 1 4 0v.1a1.7 1.7 0 0 0 2.9 1.2l.1-.1a2 2 0 1 1 2.8 2.8l-.1.1a1.7 1.7 0 0 0-.3 1.9V8a1.7 1.7 0 0 0 1.5 1H22a2 2 0 1 1 0 4h-.1a1.7 1.7 0 0 0-1.5 1Z"
          />
        </svg>
        个人设置
      </button>
      <button type="button" class="portal-account__mi" @click="comingSoon">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
        >
          <rect x="4" y="11" width="16" height="9" rx="2" />
          <path d="M8 11V7a4 4 0 0 1 8 0v4" />
        </svg>
        修改密码
      </button>
      <div class="portal-account__divider"></div>
      <button
        type="button"
        class="portal-account__mi portal-account__mi--danger"
        @click="onLogout"
      >
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
        >
          <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4" />
          <path d="m16 17 5-5-5-5" />
          <path d="M21 12H9" />
        </svg>
        登出账号
      </button>
    </div>

    <button type="button" class="portal-account__btn">
      <span class="portal-account__avatar">
        <img
          v-if="accountHeadImage"
          :src="accountHeadImage"
          :alt="accountName"
        />
        <span v-else>{{ accountInitial }}</span>
      </span>
      <span class="portal-account__name">{{ accountName }}</span>
      <svg
        class="portal-account__chev"
        viewBox="0 0 24 24"
        fill="none"
        stroke="currentColor"
        stroke-width="2"
      >
        <path d="m6 9 6 6 6-6" />
      </svg>
    </button>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent, ref } from "vue";
import { useStore } from "vuex";
import { useRouter } from "vue-router";
import { storeKey } from "@/app/store";

export default defineComponent({
  name: "PortalAccount",
  props: {
    // 无姓名信息时的兜底显示（通常传角色名）
    fallbackName: { type: String, default: "账号" },
    // 传入则「个人资料」跳转到该路由；不传则显示「开发中」（默认行为）
    profileTo: { type: String, default: "" },
  },
  emits: ["logout"],
  setup(props, { emit }) {
    const store = useStore(storeKey);
    const router = useRouter();

    // 菜单是 hover/focus 控制；点选项后用此开关立即收起，鼠标移开复位。
    const suppressed = ref(false);
    const closeMenu = () => {
      suppressed.value = true;
      (document.activeElement as HTMLElement | null)?.blur();
    };

    // 展示名：姓+名（跳过中间名）→ userName 去掉中间分隔符 → 兜底
    const accountName = computed(() => {
      const cu = store.state.currentUser;
      const composed = [cu.userLastName, cu.userFirstName]
        .filter(Boolean)
        .join("");
      if (composed) return composed;
      const stripped = (cu.userName || "").replace(/[·•・·‧\s]/g, "");
      return stripped || props.fallbackName;
    });

    const accountInitial = computed(() => {
      const cu = store.state.currentUser;
      const base = cu.userLastName || accountName.value || props.fallbackName;
      return (base || "?").charAt(0);
    });

    const accountHeadImage = computed(
      () => store.state.currentUser.userHeadImage || ""
    );

    const comingSoon = () => {
      closeMenu();
      window.alert("该功能开发中");
    };

    const goProfile = () => {
      closeMenu();
      if (props.profileTo) {
        void router.push(props.profileTo);
      } else {
        window.alert("该功能开发中");
      }
    };

    const onLogout = () => {
      closeMenu();
      emit("logout");
    };

    return {
      accountName,
      accountInitial,
      accountHeadImage,
      suppressed,
      comingSoon,
      goProfile,
      onLogout,
    };
  },
});
</script>

<style scoped>
.portal-account {
  position: relative;
  margin-top: auto;
}

.portal-account__menu {
  position: absolute;
  left: 0;
  right: 0;
  bottom: calc(100% + 8px);
  background: #fff;
  border: 1px solid #e7e9ee;
  border-radius: 12px;
  box-shadow: 0 12px 28px rgba(16, 24, 40, 0.12);
  padding: 6px;
  opacity: 0;
  visibility: hidden;
  transform: translateY(6px);
  transition: opacity 0.15s ease, transform 0.15s ease, visibility 0.15s;
  z-index: 20;
}

/* 透明桥，避免按钮与菜单间隙导致 hover 断开 */
.portal-account::after {
  content: "";
  position: absolute;
  left: 0;
  right: 0;
  bottom: 100%;
  height: 8px;
}

.portal-account:hover .portal-account__menu,
.portal-account:focus-within .portal-account__menu {
  opacity: 1;
  visibility: visible;
  transform: translateY(0);
}

/* 点选项后立即收起，优先级高于 hover/focus 展开 */
.portal-account__menu--hidden,
.portal-account:hover .portal-account__menu--hidden,
.portal-account:focus-within .portal-account__menu--hidden {
  opacity: 0;
  visibility: hidden;
  transform: translateY(6px);
  pointer-events: none;
}

.portal-account__mi {
  width: 100%;
  display: flex;
  align-items: center;
  gap: 10px;
  height: 38px;
  padding: 0 10px;
  border: 0;
  border-radius: 8px;
  background: transparent;
  color: #0f172a;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  text-align: left;
}

.portal-account__mi:hover {
  background: #f6f7f9;
}

.portal-account__mi svg {
  width: 16px;
  height: 16px;
  color: #64748b;
}

.portal-account__divider {
  height: 1px;
  background: #e7e9ee;
  margin: 6px 4px;
}

.portal-account__mi--danger {
  color: #dc2626;
}

.portal-account__mi--danger svg {
  color: #dc2626;
}

.portal-account__mi--danger:hover {
  background: #fef2f2;
}

.portal-account__btn {
  width: 100%;
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 10px;
  border: 1px solid #e7e9ee;
  border-radius: 12px;
  background: #fafbfc;
  cursor: pointer;
  text-align: left;
}

.portal-account__btn:hover {
  background: #f1f2f5;
}

.portal-account__avatar {
  width: 34px;
  height: 34px;
  border-radius: 50%;
  flex: 0 0 auto;
  overflow: hidden;
  background: linear-gradient(135deg, #6366f1, #4f46e5);
  color: #fff;
  display: grid;
  place-items: center;
  font-weight: 700;
  font-size: 14px;
}

.portal-account__avatar img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.portal-account__name {
  flex: 1;
  min-width: 0;
  font-size: 13px;
  font-weight: 700;
  color: #0f172a;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.portal-account__chev {
  width: 16px;
  height: 16px;
  color: #94a3b8;
  flex: 0 0 auto;
}
</style>
