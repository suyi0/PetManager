<template>
  <section class="page">
    <div class="panel control">
      <h3>医生考勤管理</h3>
      <div class="filters">
        <input v-model="date" type="date" />
        <select v-model="identifier">
          <option value="check_in_time">签到时间</option>
          <option value="check_out_time">签退时间</option>
        </select>
        <input
          v-model.number="userId"
          type="number"
          min="1"
          placeholder="医生ID"
        />
        <button @click="changeTime">按系统配置更新时间</button>
        <button class="ghost" @click="loadRecords">刷新列表</button>
      </div>
      <p>{{ message }}</p>
    </div>

    <div class="panel">
      <table>
        <thead>
          <tr>
            <th>医生ID</th>
            <th>姓名</th>
            <th>日期</th>
            <th>签到</th>
            <th>签退</th>
            <th>来源</th>
            <th>状态</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="item in records"
            :key="`${item.source}-${item.id}-${item.user_id}-${item.date}`"
          >
            <td>{{ item.user_id }}</td>
            <td>{{ item.name }}</td>
            <td>{{ item.date }}</td>
            <td>{{ item.check_in_time }}</td>
            <td>{{ item.check_out_time }}</td>
            <td>{{ item.source }}</td>
            <td>{{ item.status }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

export default defineComponent({
  name: "SuperAdminWorktime",
  setup() {
    const store = useStore(storeKey);
    const records = computed(() => store.state.superAdmin.workTimeRecords);
    const message = ref("等待操作");
    const userId = ref<number | null>(null);
    const identifier = ref<"check_in_time" | "check_out_time">("check_in_time");
    const date = ref(new Date().toISOString().slice(0, 10));

    /**
     * 优先复用考勤缓存。
     */
    const loadRecords = async () => {
      await store.dispatch("superAdmin/ensureWorkTimeRecords", {
        force: true,
      });
    };

    const changeTime = async () => {
      if (!userId.value) {
        message.value = "请先输入医生ID";
        return;
      }

      try {
        await store.dispatch("superAdmin/changeDoctorWorkTime", {
          user_id: userId.value,
          date: date.value,
          identifier: identifier.value,
        });
        message.value = "更新时间成功";
      } catch (err: unknown) {
        message.value = `更新时间失败: ${String(
          (err as Error).message || err
        )}`;
      }
    };

    onMounted(loadRecords);

    return {
      records,
      message,
      userId,
      identifier,
      date,
      changeTime,
      loadRecords,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  gap: 16px;
}

.panel {
  border: 1px solid #dce7ff;
  border-radius: 16px;
  background: #fff;
  padding: 16px;
}

.filters {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

input,
select {
  border: 1px solid #cfdcff;
  border-radius: 10px;
  padding: 9px 10px;
  background: #fff;
}

button {
  border: 0;
  border-radius: 10px;
  padding: 9px 12px;
  background: #2f6ff3;
  color: #fff;
  cursor: pointer;
}

button.ghost {
  background: #edf2ff;
  color: #284181;
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
</style>
