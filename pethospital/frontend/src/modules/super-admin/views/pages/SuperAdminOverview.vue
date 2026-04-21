<template>
  <section class="page">
    <div class="grid stats">
      <StatCard label="注册用户" :value="userCount" @click="toUsersPage" />
      <StatCard
        label="在线医生"
        :value="onlineCount"
        @click="toOnlineDoctorsPage"
      />
      <StatCard label="今日日志" :value="logCount" @click="toLogsPage" />
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";
import StatCard from "../../components/StatCard.vue";
import { useRoute, useRouter } from "vue-router";

export default defineComponent({
  name: "SuperAdminOverview",
  components: { StatCard },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const route = useRoute();
    const summary = computed(() => store.state.superAdmin.homePageData);
    const userCount = computed(() => summary.value.userCount);
    const onlineCount = computed(() => summary.value.onlineDoctorCount);
    const logCount = computed(() => summary.value.logsCount);

    const routePrefix = computed(() =>
      route.path.startsWith("/preview/super-admin")
        ? "/preview/super-admin"
        : "/super-admin"
    );

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
    });

    return {
      userCount,
      onlineCount,
      logCount,
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

@media (max-width: 960px) {
  .grid.stats {
    grid-template-columns: 1fr;
  }
}
</style>
