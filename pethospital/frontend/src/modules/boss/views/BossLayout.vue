<template>
  <div class="saas-shell">
    <aside class="saas-sidebar">
      <div class="saas-brand">
        <div class="saas-brand__logo">宠</div>
        <div>
          <div class="saas-brand__name">宠物医院</div>
          <div class="saas-brand__sub">总裁端</div>
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
            <path d="M3 3v18h18" />
            <path d="m7 14 4-4 3 3 5-6" />
          </svg>
          股份概览
        </RouterLink>
      </nav>

      <section class="switcher">
        <p class="switcher__title">快捷入口</p>
        <button
          v-for="portal in portalEntries"
          :key="portal.key"
          type="button"
          class="switcher__btn"
          @click="enterPortal(portal)"
        >
          <strong>{{ portal.label }}</strong>
          <span>{{ portal.hint }}</span>
        </button>
      </section>

      <PortalAccount
        fallback-name="总裁"
        profile-to="/boss/personal"
        @logout="logout"
      />
    </aside>

    <main class="saas-content">
      <RouterView />
    </main>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted } from "vue";
import { useRoute, useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { authStorage } from "@/core/auth/utils/authStorage";
import PortalAccount from "@/shared/components/PortalAccount.vue";

type PortalEntry = {
  key: string;
  label: string;
  hint: string;
  userType: number;
  userRole: string;
  path: string;
  profile: {
    userName: string;
    userPhone: string;
    userEmail: string;
    userBirthday: string;
    userAddress: string;
    userHeadImage: string;
  };
};

export default defineComponent({
  name: "BossLayout",
  components: { PortalAccount },
  setup() {
    const router = useRouter();
    const route = useRoute();
    const store = useStore(storeKey);

    const routePrefix = computed(() => "/boss");

    onMounted(() => {
      authStorage.clearBossPortalReturn();
    });

    const logout = () => {
      void store.dispatch("auth/logout");
    };

    const portalEntries = computed<PortalEntry[]>(() => [
      {
        key: "super-admin",
        label: "进入管理员端",
        hint: "查看总览、用户、考勤与日志审计",
        userType: 7,
        userRole: "超级管理员",
        path: "/super-admin/overview",
        profile: {
          userName: "超级管理员 沈知序",
          userPhone: "13600136000",
          userEmail: "super.admin@example.com",
          userBirthday: "1988-02-14",
          userAddress: "上海市徐汇区宠物医院总部 18 层",
          userHeadImage: "",
        },
      },
      {
        key: "personnel",
        label: "进入人事端",
        hint: "打开权限授予与岗位管理界面",
        userType: 5,
        userRole: "人事经理",
        path: "/personnel/access",
        profile: {
          userName: "人事经理 许知夏",
          userPhone: "13600136001",
          userEmail: "personnel.xu@example.com",
          userBirthday: "1992-08-19",
          userAddress: "上海市徐汇区人事管理中心 8 楼",
          userHeadImage: "",
        },
      },
      {
        key: "warehouse",
        label: "进入仓库端",
        hint: "查看库存仪表盘、预警与出入库记录",
        userType: 8,
        userRole: "仓库管理员",
        path: "/warehouse-admin/dashboard",
        profile: {
          userName: "仓库管理员 陈序",
          userPhone: "13700137000",
          userEmail: "warehouse.chen@example.com",
          userBirthday: "1993-11-22",
          userAddress: "苏州市工业园区补给仓 A-03",
          userHeadImage: "",
        },
      },
      {
        key: "doctor",
        label: "进入医生端",
        hint: "打开工作台、候诊队列与开单流程",
        userType: 9,
        userRole: "医生",
        path: "/doctor/home",
        profile: {
          userName: "值班医生 周予安",
          userPhone: "13900139000",
          userEmail: "doctor.zhou@example.com",
          userBirthday: "1991-03-09",
          userAddress: "杭州市滨江区宠医中心 6 楼",
          userHeadImage: "",
        },
      },
      {
        key: "user",
        label: "进入用户端",
        hint: "查看首页、服务预约与个人资料",
        userType: 11,
        userRole: "普通用户",
        path: "/user/home",
        profile: {
          userName: "体验用户 林小满",
          userPhone: "13800138000",
          userEmail: "pet.user@example.com",
          userBirthday: "1998-06-18",
          userAddress: "上海市浦东新区爪印大道 18 号",
          userHeadImage: "",
        },
      },
    ]);

    const enterPortal = async (portal: PortalEntry) => {
      authStorage.saveBossPortalReturn({
        returnTo: route.fullPath || `${routePrefix.value}/overview`,
      });

      await router.push(portal.path);
    };

    return {
      routePrefix,
      portalEntries,
      enterPortal,
      logout,
    };
  },
});
</script>

<style scoped>
/* 外壳来自全局 saas.css；保留“快捷入口”枢纽，内容区自然滚动。 */
.saas-content {
  display: block;
  overflow-y: auto;
}

.switcher {
  display: grid;
  gap: 8px;
  margin-top: 14px;
}

.switcher__title {
  margin: 0 0 2px;
  color: #94a3b8;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.switcher__btn {
  display: grid;
  gap: 3px;
  width: 100%;
  padding: 10px 12px;
  border: 1px solid #e7e9ee;
  border-radius: 10px;
  text-align: left;
  cursor: pointer;
  color: #0f172a;
  background: #fafbfc;
  transition: background 0.15s ease, border-color 0.15s ease;
}

.switcher__btn:hover {
  background: #f1f2f5;
  border-color: #4f46e5;
}

.switcher__btn strong {
  font-size: 13px;
  font-weight: 700;
}

.switcher__btn span {
  font-size: 12px;
  color: #64748b;
  line-height: 1.4;
}
</style>
