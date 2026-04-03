<template>
  <section class="page">
    <div class="grid stats">
      <StatCard
        label="注册用户"
        :value="users.length"
        hint="来自 /api/admin/getUsers"
      />
      <StatCard
        label="当日打卡医生"
        :value="onlineCount"
        hint="source = online_doctors"
      />
      <StatCard
        label="历史考勤条数"
        :value="historyCount"
        hint="source = work_records"
      />
    </div>

    <div class="panel">
      <div class="head">
        <h3>最近考勤（前 8 条）</h3>
        <button @click="loadAll">刷新</button>
      </div>
      <table>
        <thead>
          <tr>
            <th>姓名</th>
            <th>日期</th>
            <th>签到</th>
            <th>签退</th>
            <th>状态</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="item in previewRecords"
            :key="`${item.source}-${item.id}-${item.user_id}-${item.date}`"
          >
            <td>{{ item.name }}</td>
            <td>{{ item.date }}</td>
            <td>{{ item.check_in_time }}</td>
            <td>{{ item.check_out_time }}</td>
            <td>{{ item.status }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref } from "vue";
import StatCard from "../../components/StatCard.vue";
import { superAdminApi } from "../../api/superAdminApi";
import { UserRow, WorkTimeRecord } from "../../api/types";

export default defineComponent({
  name: "SuperAdminOverview",
  components: { StatCard },
  setup() {
    const users = ref<UserRow[]>([]);
    const records = ref<WorkTimeRecord[]>([]);

    const onlineCount = computed(
      () => records.value.filter((it) => it.source === "online_doctors").length
    );

    const historyCount = computed(
      () => records.value.filter((it) => it.source === "work_records").length
    );

    const previewRecords = computed(() => records.value.slice(0, 8));

    const loadAll = async () => {
      const [userRows, workRows] = await Promise.all([
        superAdminApi.getUsers(),
        superAdminApi.getWorkTimeRecord(),
      ]);
      users.value = userRows;
      records.value = workRows;
    };

    onMounted(loadAll);

    return {
      users,
      onlineCount,
      historyCount,
      previewRecords,
      loadAll,
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
