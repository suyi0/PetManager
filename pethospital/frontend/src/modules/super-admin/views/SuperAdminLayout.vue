<template>
  <div class="shell">
    <aside class="sidebar">
      <div class="sidebar-head">{{ currentRoleLabel }}</div>
      <div class="sidebar-logo"></div>
      <nav>
        <RouterLink :to="`${routePrefix}/overview`">总览</RouterLink>
        <RouterLink :to="`${routePrefix}/worktime`">考勤管理</RouterLink>
        <RouterLink :to="`${routePrefix}/users`">用户管理</RouterLink>
        <RouterLink :to="`${routePrefix}/online-doctors`">在线医生</RouterLink>
        <RouterLink :to="`${routePrefix}/logs`">日志审计</RouterLink>
      </nav>
      <section class="portal-switcher">
        <p class="portal-switcher__title">快捷进入其他端</p>
        <button
          v-for="portal in crossPortals"
          :key="portal.key"
          type="button"
          class="portal-switcher__button"
          @click="enterPortal(portal)"
        >
          <strong>{{ portal.label }}</strong>
          <span>{{ portal.hint }}</span>
        </button>
      </section>
    </aside>

    <main class="content">
      <header class="topbar">
        <h2>宠物医院管理端</h2>
        <span>接口驱动 · Crow + MySQL</span>
        <button @click="logout" class="lgout"><span>登出</span></button>
      </header>
      <RouterView />
    </main>
  </div>
</template>

<script lang="ts">
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";
import { computed, defineComponent, onBeforeUnmount, onMounted } from "vue";
import { useRoute, useRouter } from "vue-router";
import { authStorage } from "@/core/auth/utils/authStorage";
import { isSuperAdminPortalRole } from "@/core/auth/utils/roleUtils";
import {
  startSuperAdminSessionGuard,
  stopSuperAdminSessionGuard,
} from "@/modules/super-admin/utils/superAdminSessionGuard";

export default defineComponent({
  name: "SuperAdminLayout",
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const route = useRoute();

    const createFrontendToken = (userType: number, userRole: string) => {
      const encode = (value: string) =>
        btoa(value).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/g, "");

      const header = encode(JSON.stringify({ alg: "none", typ: "JWT" }));
      const payload = encode(
        JSON.stringify({
          exp: Math.floor(Date.now() / 1000) + 7 * 24 * 60 * 60,
          type_id: userType,
          type_name: userRole,
        })
      );

      return `${header}.${payload}.frontend`;
    };

    onMounted(() => {
      startSuperAdminSessionGuard(store, router);
      // 超级管理员进入任意页面时，先预热总览需要的基础数据。
      void store.dispatch("superAdmin/ensureOverviewData");
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

    const routePrefix = computed(() =>
      route.path.startsWith("/preview/super-admin")
        ? "/preview/super-admin"
        : "/super-admin"
    );

    const isPreviewRoute = computed(() =>
      route.path.startsWith("/preview/super-admin")
    );

    const currentRoleLabel = computed(() => {
      const activeRole = store.state.auth.userRole;
      return activeRole && isSuperAdminPortalRole(activeRole)
        ? activeRole
        : "超级管理员";
    });

    const crossPortals = computed(() => [
      {
        key: "user",
        label: "进入用户端",
        hint: "写入用户基础资料并跳转首页",
        userType: 4,
        userRole: "普通用户",
        to: isPreviewRoute.value ? "/preview/user/home" : "/user/home",
        profile: {
          userName: "体验用户 林小满",
          userPhone: "13800138000",
          userEmail: "pet.user@example.com",
          userBirthday: "1998-06-18",
          userAddressId: "preview-user-address",
          userAddress: "上海市浦东新区爪印大道 18 号",
          userHeadImage: "",
        },
      },
      {
        key: "doctor",
        label: "进入医生端",
        hint: "写入医生基础资料并打开工作台",
        userType: 2,
        userRole: "医生",
        to: isPreviewRoute.value ? "/preview/doctor/home" : "/doctor/home",
        profile: {
          userName: "值班医生 周予安",
          userPhone: "13900139000",
          userEmail: "doctor.zhou@example.com",
          userBirthday: "1991-03-09",
          userAddressId: "preview-doctor-address",
          userAddress: "杭州市滨江区宠医中心 6 楼",
          userHeadImage: "",
        },
      },
      {
        key: "warehouse",
        label: "进入仓库端",
        hint: "写入仓库管理员资料并打开仪表盘",
        userType: 3,
        userRole: "仓库管理员",
        to: isPreviewRoute.value
          ? "/preview/warehouse-admin/dashboard"
          : "/warehouse-admin/dashboard",
        profile: {
          userName: "仓库管理员 陈序",
          userPhone: "13700137000",
          userEmail: "warehouse.chen@example.com",
          userBirthday: "1993-11-22",
          userAddressId: "preview-warehouse-address",
          userAddress: "苏州市工业园区补给仓 A-03",
          userHeadImage: "",
        },
      },
    ]);

    const enterPortal = async (portal: (typeof crossPortals.value)[number]) => {
      const currentRole =
        store.state.auth.userRole &&
        isSuperAdminPortalRole(store.state.auth.userRole)
          ? store.state.auth.userRole
          : "超级管理员";
      const currentType = store.state.auth.userType || 5;
      const currentProfile = {
        userName: store.state.currentUser.userName || `${currentRole} 沈知序`,
        userPhone: store.state.currentUser.userPhone || "13600136000",
        userEmail:
          store.state.currentUser.userEmail || "super.admin@example.com",
        userBirthday: store.state.currentUser.userBirthday || "1988-02-14",
        userAddressId:
          store.state.currentUser.userAddressId || "super-admin-address",
        userAddress:
          store.state.currentUser.userAddress ||
          "上海市徐汇区宠物医院总部 18 层",
        userHeadImage: store.state.currentUser.userHeadImage || "",
      };
      const nextToken =
        !isPreviewRoute.value && store.state.auth.token
          ? store.state.auth.token
          : createFrontendToken(portal.userType, portal.userRole);
      const returnToken =
        store.state.auth.token || createFrontendToken(currentType, currentRole);

      authStorage.saveAdminPortalBridge({
        returnTo: isPreviewRoute.value
          ? "/preview/super-admin/overview"
          : "/super-admin/overview",
        token: returnToken,
        userType: currentType,
        userRole: currentRole,
        ...currentProfile,
      });

      store.commit("auth/setSession", {
        token: nextToken,
        userType: portal.userType,
        userRole: portal.userRole,
      });
      store.commit(
        "currentUser/setCurrentUser",
        {
          ...portal.profile,
          userType: portal.userType,
          userRole: portal.userRole,
        },
        { root: true }
      );

      await router.push(portal.to);
    };

    return {
      logout,
      routePrefix,
      crossPortals,
      currentRoleLabel,
      enterPortal,
    };
  },
});
</script>

<style scoped>
.shell {
  display: grid;
  grid-template-columns: 268px 1fr;
  min-height: 100vh;
  background: radial-gradient(
    circle at 20% 10%,
    #f5f9ff 0%,
    #f0f4ff 45%,
    #e9eefb 100%
  );
  color: #182442;
  font-family: "PingFang SC", "Segoe UI", sans-serif;
}

.sidebar {
  top: 0;
  height: 100vh;
  min-height: 100vh;
  position: sticky;
  display: flex;
  flex-direction: column;
  align-self: stretch;
  padding: 32px 20px 24px;
  border-right: 1px solid #d5e2ff;
  background: linear-gradient(180deg, #ffffff 0%, #f7faff 100%);
  box-shadow: inset -1px 0 0 rgba(255, 255, 255, 0.8);
  z-index: 9999;

  .sidebar-head {
    display: flex;
    align-items: center;
    justify-content: center;
    min-height: 72px;
    margin-bottom: 18px;
    padding: 0 18px;
    font-weight: 700;
    font-size: 30px;
    letter-spacing: 0.04em;
    color: #17345f;
    background: linear-gradient(135deg, #4fd4d6 0%, #2ca8d5 100%);
    border: 1px solid rgba(52, 144, 196, 0.24);
    border-radius: 26px;
    box-shadow: 0 16px 30px rgba(45, 125, 186, 0.16);
  }
}

.sidebar-logo {
  height: 1px;
  margin: 0 8px 18px;
  background: linear-gradient(
    90deg,
    rgba(90, 122, 178, 0),
    rgba(90, 122, 178, 0.42),
    rgba(90, 122, 178, 0)
  );
}

.sidebar nav {
  display: grid;
  gap: 10px;
  padding: 12px;
  border: 1px solid #d9e5fb;
  border-radius: 22px;
  background: linear-gradient(180deg, rgba(245, 248, 255, 0.96), #ffffff);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 14px 28px rgba(97, 122, 168, 0.08);
}

.sidebar a {
  position: relative;
  text-decoration: none;
  color: #415177;
  padding: 14px 16px 14px 18px;
  border: 1px solid #e4ebfb;
  border-radius: 16px;
  background: linear-gradient(180deg, #ffffff, #f9fbff);
  font-size: 15px;
  font-weight: 600;
  transition: 0.2s ease;
  box-shadow: 0 8px 18px rgba(117, 138, 181, 0.06);
}

.sidebar a::before {
  content: "";
  position: absolute;
  left: 10px;
  top: 50%;
  width: 4px;
  height: calc(100% - 20px);
  border-radius: 999px;
  background: linear-gradient(180deg, #8fb5ff, #bdd0ff);
  opacity: 0;
  transform: translateY(-50%);
  transition: 0.2s ease;
}

.sidebar a:hover {
  color: #243559;
  border-color: #cfdcf7;
  transform: translateX(2px);
  box-shadow: 0 12px 24px rgba(94, 124, 177, 0.12);
}

.sidebar a.router-link-active {
  color: #16335c;
  border-color: rgba(85, 146, 232, 0.34);
  background: linear-gradient(135deg, #e8f7ff 0%, #dff1ff 52%, #eef6ff 100%);
  box-shadow: 0 16px 28px rgba(75, 126, 186, 0.16);
}

.sidebar a.router-link-active::before {
  opacity: 1;
}

.sidebar a + a {
  margin-top: 2px;
}

.portal-switcher {
  margin-top: auto;
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid #d9e5fb;
  border-radius: 22px;
  background: linear-gradient(180deg, rgba(238, 247, 255, 0.96), #ffffff);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 14px 28px rgba(97, 122, 168, 0.08);
}

.portal-switcher__title {
  margin: 0 0 2px;
  color: #5b6f98;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.08em;
}

.portal-switcher__button {
  display: grid;
  gap: 4px;
  width: 100%;
  padding: 14px 16px;
  text-align: left;
  border: 1px solid #dbe7ff;
  border-radius: 16px;
  background: linear-gradient(135deg, #f9fcff 0%, #ecf5ff 100%);
  color: #23406d;
  cursor: pointer;
  transition: 0.2s ease;
  box-shadow: 0 10px 22px rgba(108, 134, 184, 0.08);
}

.portal-switcher__button strong {
  font-size: 14px;
}

.portal-switcher__button span {
  font-size: 12px;
  color: #7184aa;
}

.portal-switcher__button:hover {
  transform: translateY(-1px);
  border-color: #c8dbff;
  box-shadow: 0 14px 26px rgba(91, 128, 191, 0.14);
}

.content {
  min-width: 0;
  height: 100vh;
  min-height: 100vh;
  overflow-x: hidden;
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  padding: 20px 26px 32px;
  box-sizing: border-box;
}

.topbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 18px;
}

.topbar h2 {
  margin: 0;
  font-size: 24px;
}

.topbar span {
  font-size: 12px;
  color: #6c7c9f;
}

.lgout {
  border-radius: 999px;
  background: linear-gradient(135deg, #9debe4, #19aaaf);

  span {
    font-size: 18px;
  }
}

@media (max-width: 960px) {
  .shell {
    grid-template-columns: 1fr;
  }

  .sidebar {
    top: auto;
    height: auto;
    min-height: auto;
    position: relative;
    border-right: 0;
    border-bottom: 1px solid #d5e2ff;
  }

  .sidebar nav {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .portal-switcher {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .content {
    height: auto;
    min-height: 0;
    overflow-y: visible;
  }
}

@media (max-width: 640px) {
  .sidebar nav,
  .portal-switcher {
    grid-template-columns: 1fr;
  }
}
</style>
