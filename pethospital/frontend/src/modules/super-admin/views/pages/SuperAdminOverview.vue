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
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onBeforeUnmount, onMounted } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import StatCard from "../../components/StatCard.vue";
import { useRouter } from "vue-router";
import { saveSuperAdminHomePageDataCache } from "../../utils/superAdminDataCache";
import { subscribeSuperAdminHomeData } from "../../utils/superAdminHomeDataStream";

export default defineComponent({
  name: "SuperAdminOverview",
  components: { StatCard },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    let closeHomeDataStream: (() => void) | null = null;
    const summary = computed(() => store.state.superAdmin.homePageData);
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
    const toOnlineDoctorsPage = () =>
      router.push(`${routePrefix.value}/online-doctors`);
    const toLogsPage = () => router.push(`${routePrefix.value}/logs`);

    /**
     * 手动刷新首页摘要卡片。
     */
    const loadAll = async () => {
      await store.dispatch("superAdmin/refreshOverviewData");
    };

    onMounted(() => {
      // 页面首次进入时优先复用首页摘要缓存，只有过期或脏数据时才会重拉。
      void store.dispatch("superAdmin/ensureOverviewData");

      closeHomeDataStream = subscribeSuperAdminHomeData((homeData) => {
        store.commit("superAdmin/setHomePageData", homeData);
        saveSuperAdminHomePageDataCache(homeData);
      });
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
      expenseAmount,
      salesAmount,
      costAmount,
      profitAmount,
      loadAll,
      toUsersPage,
      toOnlineDoctorsPage,
      toLogsPage,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  gap: 16px;
}

.grid.stats {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
}

.panel {
  border: 1px solid #dce7ff;
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
  background: #2f6ff3;
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
  border-bottom: 1px solid #edf2ff;
}

.statCard {
  cursor: pointer;
}

@media (max-width: 960px) {
  .grid.stats {
    grid-template-columns: 1fr;
  }
}
</style>
