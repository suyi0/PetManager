<template>
  <div class="boss-shell">
    <aside class="boss-sidebar">
      <div class="boss-sidebar__tag">Boss Portal</div>
      <h1 class="boss-sidebar__title">股份中枢</h1>

      <nav class="boss-nav">
        <RouterLink :to="`${routePrefix}/overview`">股份概览</RouterLink>
      </nav>

      <section class="boss-portal-switcher">
        <p class="boss-portal-switcher__title">快捷入口</p>
        <button
          v-for="portal in portalEntries"
          :key="portal.key"
          type="button"
          class="boss-portal-switcher__button"
          @click="enterPortal(portal)"
        >
          <strong>{{ portal.label }}</strong>
          <span>{{ portal.hint }}</span>
        </button>
      </section>
    </aside>

    <main class="boss-content">
      <header class="boss-topbar">
        <div>
          <h2>公司股份总览</h2>
        </div>
      </header>
      <RouterView />
    </main>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted } from "vue";
import { useRoute, useRouter } from "vue-router";
import { authStorage } from "@/core/auth/utils/authStorage";

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
  setup() {
    const router = useRouter();
    const route = useRoute();

    const routePrefix = computed(() => "/boss");

    onMounted(() => {
      authStorage.clearBossPortalReturn();
    });

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
      // {
      //   key: "finance",
      //   label: "进入财务端",
      //   hint: "打开薪资与财务管理界面",
      //   userType: 3,
      //   userRole: "财务经理",
      //   path: "/finance/salary",
      //   profile: {
      //     userName: "财务经理 林予澄",
      //     userPhone: "13600136002",
      //     userEmail: "finance.lin@example.com",
      //     userBirthday: "1990-04-26",
      //     userAddress: "上海市徐汇区财务管理中心 12 楼",
      //     userHeadImage: "",
      //   },
      // },
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
    };
  },
});
</script>

<style scoped>
.boss-shell {
  display: grid;
  grid-template-columns: 290px 1fr;
  min-height: 100vh;
  background: radial-gradient(
      circle at top right,
      rgba(255, 241, 220, 0.95),
      transparent 28%
    ),
    linear-gradient(135deg, #f8f2e8 0%, #efe3cf 48%, #e6d6bc 100%);
  color: #261b13;
  font-family: "PingFang SC", "Segoe UI", sans-serif;
}

.boss-sidebar {
  position: sticky;
  top: 0;
  height: 100vh;
  padding: 34px 24px;
  display: flex;
  flex-direction: column;
  gap: 18px;
  border-right: 1px solid rgba(90, 66, 42, 0.12);
  background: linear-gradient(
    180deg,
    rgba(255, 252, 246, 0.92),
    rgba(248, 240, 224, 0.92)
  );
  backdrop-filter: blur(14px);
}

.boss-sidebar__tag {
  width: fit-content;
  padding: 8px 12px;
  border-radius: 999px;
  background: #f4e6ce;
  color: #896848;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.boss-sidebar__title {
  margin: 0;
  font-family: Georgia, serif;
  font-size: 42px;
  line-height: 1;
}

.boss-sidebar__subtitle {
  margin: 0;
  color: #7c6753;
  line-height: 1.7;
  font-size: 14px;
}

.boss-nav {
  display: grid;
  gap: 10px;
  margin-top: 14px;
}

.boss-nav a {
  padding: 14px 16px;
  border-radius: 16px;
  color: #5c4630;
  text-decoration: none;
  background: rgba(255, 255, 255, 0.52);
  font-weight: 700;
}

.boss-nav a.router-link-active {
  background: linear-gradient(135deg, #fff8ef, #f5e5cc);
  box-shadow: 0 12px 28px rgba(101, 76, 49, 0.12);
}

.boss-portal-switcher {
  margin-top: auto;
  display: grid;
  gap: 10px;
}

.boss-portal-switcher__title {
  margin: 0;
  color: #8e7359;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.boss-portal-switcher__button {
  display: grid;
  gap: 6px;
  width: 100%;
  padding: 16px 18px;
  border-radius: 18px;
  border: 0;
  text-align: left;
  text-decoration: none;
  cursor: pointer;
  color: #5a422c;
  background: linear-gradient(135deg, #fffaf2, #f5e4cb);
  box-shadow: 0 14px 30px rgba(117, 87, 56, 0.12);
  transition: transform 0.18s ease, box-shadow 0.18s ease;
}

.boss-portal-switcher__button strong {
  font-size: 15px;
}

.boss-portal-switcher__button span {
  color: #876c52;
  font-size: 12px;
  line-height: 1.5;
}

.boss-portal-switcher__button:hover {
  transform: translateY(-2px);
  box-shadow: 0 18px 34px rgba(117, 87, 56, 0.16);
}

.boss-content {
  padding: 28px 32px 36px;
}

.boss-topbar {
  margin-bottom: 20px;
}

.boss-topbar h2 {
  margin: 0 0 6px;
  font-size: 28px;
}

.boss-topbar span {
  color: #7c6753;
  font-size: 14px;
}

@media (max-width: 1080px) {
  .boss-shell {
    grid-template-columns: 1fr;
  }

  .boss-sidebar {
    position: relative;
    height: auto;
  }
}
</style>
