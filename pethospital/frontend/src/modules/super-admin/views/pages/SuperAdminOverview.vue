<template>
  <section class="page">
    <div class="grid stats">
      <StatCard
        class="statCard"
        label="注册用户"
        :value="userCount"
        hint="当前系统账号总量"
        @click="toUsersPage"
      />
      <StatCard
        class="statCard"
        label="在线医生"
        :value="onlineDoctorCount"
        hint="实时值班接诊医生"
        @click="toOnlineDoctorsPage"
      />
      <StatCard
        class="statCard"
        label="每日开销"
        :value="expenseAmount"
        hint="员工工资每日摊销"
      />
      <StatCard
        class="statCard"
        label="全部日志"
        :value="allLogCount"
        hint="用户与系统日志总量"
        @click="toLogsPage"
      />
      <StatCard
        class="statCard"
        label="每日营业额"
        :value="salesAmount"
        hint="今日订单收入"
      />
      <StatCard
        class="statCard"
        label="每日成本"
        :value="costAmount"
        hint="工资摊销与药品成本"
      />
      <StatCard
        class="statCard"
        label="每日利润"
        :value="profitAmount"
        hint="营业额减成本"
      />
      <StatCard
        class="statCard"
        label="用户日志"
        :value="userLogCount"
        hint="用户行为日志总量"
        @click="toLogsPage"
      />
      <StatCard
        class="statCard"
        label="系统日志"
        :value="systemLogCount"
        hint="系统运行日志总量"
        @click="toLogsPage"
      />
    </div>

    <div class="panel hub">
      <div class="hub__head">
        <h3>快捷入口</h3>
        <span>常用管理操作直达</span>
      </div>
      <div class="hub__grid">
        <button class="hub-tile" type="button" @click="toUsersPage">
          <span class="hub-tile__icon hub-tile__icon--users">👥</span>
          <span class="hub-tile__body">
            <strong>用户管理</strong>
            <small>账号、角色与资料台账</small>
          </span>
          <span class="hub-tile__arrow">→</span>
        </button>
        <button class="hub-tile" type="button" @click="toWorktimePage">
          <span class="hub-tile__icon hub-tile__icon--time">⏱</span>
          <span class="hub-tile__body">
            <strong>考勤管理</strong>
            <small>医生签到签退记录</small>
          </span>
          <span class="hub-tile__arrow">→</span>
        </button>
        <button class="hub-tile" type="button" @click="toOnlineDoctorsPage">
          <span class="hub-tile__icon hub-tile__icon--doctor">🩺</span>
          <span class="hub-tile__body">
            <strong>在线医生</strong>
            <small>实时值班与接诊状态</small>
          </span>
          <span class="hub-tile__arrow">→</span>
        </button>
        <button class="hub-tile" type="button" @click="toLogsPage">
          <span class="hub-tile__icon hub-tile__icon--logs">📋</span>
          <span class="hub-tile__body">
            <strong>日志审计</strong>
            <small>用户与系统操作日志</small>
          </span>
          <span class="hub-tile__arrow">→</span>
        </button>
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onBeforeUnmount, onMounted } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import StatCard from "../../components/StatCard.vue";
import { useRoute, useRouter } from "vue-router";
import { subscribeSuperAdminHomeData } from "../../utils/superAdminHomeDataStream";

export default defineComponent({
  name: "SuperAdminOverview",
  components: { StatCard },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const route = useRoute();
    let closeHomeDataStream: (() => void) | null = null;
    const summary = computed(() => store.state.superAdmin.homePageData);
    const isSuperAdminHomePage = computed(
      () => route.name === "superAdminOverview"
    );
    const userCount = computed(() => summary.value.userCount);
    const onlineDoctorCount = computed(() => summary.value.onlineDoctorCount);
    const allLogCount = computed(() => summary.value.allLogCount);
    const userLogCount = computed(() => summary.value.userLogCount);
    const systemLogCount = computed(() => summary.value.systemLogCount);
    const formatCurrency = (value: number) =>
      `￥${Number(value || 0).toLocaleString("zh-CN", {
        maximumFractionDigits: 2,
      })}`;
    const expenseAmount = computed(() =>
      formatCurrency(summary.value.dailyExpense)
    );
    const salesAmount = computed(() =>
      formatCurrency(summary.value.dailySales)
    );
    const costAmount = computed(() => formatCurrency(summary.value.dailyCost));
    const profitAmount = computed(() =>
      formatCurrency(summary.value.dailyProfit)
    );

    const routePrefix = computed(() => "/super-admin");

    const toUsersPage = () => router.push(`${routePrefix.value}/users`);
    const toWorktimePage = () => router.push(`${routePrefix.value}/worktime`);
    const toOnlineDoctorsPage = () =>
      router.push(`${routePrefix.value}/online-doctors`);
    const toLogsPage = () => router.push(`${routePrefix.value}/logs`);

    /**
     * 手动刷新首页摘要卡片。
     */
    const loadAll = async () => {
      await store.dispatch("superAdmin/ensureHomePageData", { force: true });
    };

    onMounted(() => {
      // 页面首次进入时优先复用首页摘要缓存，只有过期或脏数据时才会重拉。
      void store.dispatch("superAdmin/ensureHomePageData");

      // 监听是否处于超级管理员端首页
      if (isSuperAdminHomePage.value) {
        closeHomeDataStream = subscribeSuperAdminHomeData(
          (homeData) => {
            void store.dispatch(
              "superAdmin/applyRealtimeHomePageData",
              homeData
            );
          },
          {
            onFallbackRefresh: () => {
              void loadAll();
            },
          }
        );
      }
    });

    onBeforeUnmount(() => {
      closeHomeDataStream?.();
      closeHomeDataStream = null;
    });

    return {
      userCount,
      onlineDoctorCount,
      allLogCount,
      userLogCount,
      systemLogCount,
      isSuperAdminHomePage,
      expenseAmount,
      salesAmount,
      costAmount,
      profitAmount,
      loadAll,
      toUsersPage,
      toWorktimePage,
      toOnlineDoctorsPage,
      toLogsPage,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 16px;
  height: 100%;
  min-height: 0;
  overflow-y: auto;
}

.grid.stats {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
}

/* 快捷入口：填满 KPI 下方的剩余高度，给总览页一个实用的下半区 */
.hub {
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.hub__head {
  display: flex;
  align-items: baseline;
  gap: 10px;
}

.hub__head h3 {
  margin: 0;
  font-size: 15px;
  font-weight: 700;
  color: #0f172a;
}

.hub__head span {
  font-size: 12px;
  color: #94a3b8;
}

.hub__grid {
  flex: 1;
  min-height: 0;
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
}

.hub-tile {
  display: flex;
  align-items: center;
  gap: 12px;
  text-align: left;
  padding: 16px;
  border: 1px solid #e7e9ee;
  border-radius: 14px;
  background: #fbfbfd;
  cursor: pointer;
  transition: border-color 0.15s ease, background 0.15s ease,
    box-shadow 0.15s ease;
}

.hub-tile:hover {
  background: #fff;
  border-color: #c7d2fe;
  box-shadow: 0 6px 16px rgba(47, 125, 208, 0.08);
}

.hub-tile__icon {
  display: grid;
  place-items: center;
  width: 40px;
  height: 40px;
  flex: 0 0 auto;
  border-radius: 11px;
  font-size: 18px;
}

.hub-tile__icon--users {
  background: #e2eefb;
}
.hub-tile__icon--time {
  background: #ecfeff;
}
.hub-tile__icon--doctor {
  background: #ecfdf5;
}
.hub-tile__icon--logs {
  background: #fef3f2;
}

.hub-tile__body {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
  flex: 1;
}

.hub-tile__body strong {
  font-size: 14px;
  font-weight: 700;
  color: #0f172a;
}

.hub-tile__body small {
  font-size: 12px;
  color: #64748b;
}

.hub-tile__arrow {
  color: #c7d2fe;
  font-size: 16px;
  flex: 0 0 auto;
}

.hub-tile:hover .hub-tile__arrow {
  color: #2f7dd0;
}

.panel {
  border: 1px solid #e7e9ee;
  border-radius: 16px;
  background: #fff;
  padding: 16px;
}

.head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12px;
}

.head h3 {
  margin: 0;
}

button {
  border: 0;
  padding: 8px 14px;
  border-radius: 10px;
  background: #2f7dd0;
  color: #fff;
  cursor: pointer;
}

table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

th,
td {
  text-align: left;
  padding: 10px;
  border-bottom: 1px solid #e2eefb;
}

.statCard {
  cursor: pointer;
}

@media (max-width: 960px) {
  .grid.stats {
    grid-template-columns: 1fr;
  }

  .hub__grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
</style>
