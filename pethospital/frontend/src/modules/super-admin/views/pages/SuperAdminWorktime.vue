<template>
  <section class="page">
    <div class="panel control">
      <div class="control__head">
        <h3>医生考勤管理</h3>
        <p class="status">{{ message }}</p>
      </div>
      <div class="filters">
        <input v-model="date" class="field" type="date" />
        <select v-model="identifier" class="field">
          <option value="check_in_time">签到时间</option>
          <option value="check_out_time">签退时间</option>
        </select>
        <input
          v-model.number="userId"
          class="field"
          type="number"
          min="1"
          placeholder="医生ID"
        />
        <button class="btn" @click="changeTime">按系统配置更新时间</button>
        <button class="btn btn--ghost" @click="loadRecords">刷新列表</button>
      </div>
    </div>

    <div class="panel table-panel">
      <div ref="tableShellRef" class="table-shell">
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
              <td class="mono">{{ item.user_id }}</td>
              <td>{{ item.name }}</td>
              <td>{{ item.date }}</td>
              <td>{{ item.check_in_time }}</td>
              <td>{{ item.check_out_time }}</td>
              <td>{{ item.source }}</td>
              <td>{{ item.status }}</td>
            </tr>

            <tr v-if="records.length === 0">
              <td class="empty-cell" colspan="7">暂无考勤记录</td>
            </tr>

            <tr
              v-for="n in placeholderRows"
              :key="`ph-${n}`"
              class="placeholder-row"
            >
              <td colspan="7"></td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import {
  computed,
  defineComponent,
  nextTick,
  onBeforeUnmount,
  onMounted,
  ref,
} from "vue";
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
    const tableShellRef = ref<HTMLElement | null>(null);
    const capacity = ref(8);

    const placeholderRows = computed(() =>
      records.value.length === 0
        ? 0
        : Math.max(0, capacity.value - records.value.length)
    );

    // 按表格可用高度反推可容纳行数，用占位空行补满、页面不滚动。
    const measureCapacity = () => {
      const shell = tableShellRef.value;
      if (!shell) return;
      const shellH = shell.clientHeight;
      if (!shellH) return;
      const thead = shell.querySelector("thead");
      const headerH = thead ? thead.getBoundingClientRect().height : 42;
      const firstRow = shell.querySelector("tbody tr:not(.placeholder-row)");
      const rowH = firstRow ? firstRow.getBoundingClientRect().height : 44;
      capacity.value = Math.max(4, Math.floor((shellH - headerH) / rowH));
    };

    /**
     * 优先复用考勤缓存。
     */
    const loadRecords = async () => {
      await store.dispatch("superAdmin/ensureWorkTimeRecords", {
        force: true,
      });
      void nextTick(measureCapacity);
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

    let resizeObserver: ResizeObserver | null = null;

    onMounted(() => {
      void loadRecords();
      void nextTick(() => {
        measureCapacity();
        if (tableShellRef.value && typeof ResizeObserver !== "undefined") {
          resizeObserver = new ResizeObserver(() => measureCapacity());
          resizeObserver.observe(tableShellRef.value);
        }
      });
    });

    onBeforeUnmount(() => {
      resizeObserver?.disconnect();
    });

    return {
      records,
      message,
      userId,
      identifier,
      date,
      tableShellRef,
      placeholderRows,
      changeTime,
      loadRecords,
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
}

.panel {
  border: 1px solid #e7e9ee;
  border-radius: 14px;
  background: #fff;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

.control {
  padding: 16px 18px;
  display: grid;
  gap: 14px;
}

.control__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
}

.control__head h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 700;
  color: #0f172a;
}

.status {
  margin: 0;
  font-size: 13px;
  color: #64748b;
}

.filters {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.field {
  height: 38px;
  border: 1px solid #e0e7ff;
  border-radius: 9px;
  padding: 0 12px;
  background: #fff;
  color: #0f172a;
  font-size: 13px;
}

.field:focus {
  outline: none;
  border-color: #4f46e5;
  box-shadow: 0 0 0 3px #eef2ff;
}

.btn {
  height: 38px;
  border: 1px solid transparent;
  border-radius: 9px;
  padding: 0 16px;
  background: #4f46e5;
  color: #fff;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.btn:hover {
  background: #4338ca;
}

.btn--ghost {
  background: #fff;
  border-color: #e7e9ee;
  color: #0f172a;
}

.btn--ghost:hover {
  background: #f6f7f9;
}

.table-panel {
  display: flex;
  flex-direction: column;
  min-height: 0;
  padding: 6px;
}

.table-shell {
  flex: 1;
  min-height: 0;
  overflow: hidden;
  border-radius: 10px;
}

table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
  font-size: 13px;
}

thead th {
  height: 40px;
  text-align: left;
  padding: 0 14px;
  background: #f8fafc;
  color: #64748b;
  font-size: 12px;
  font-weight: 700;
  border-bottom: 1px solid #e5e7eb;
}

tbody td {
  height: 44px;
  padding: 0 14px;
  text-align: left;
  color: #0f172a;
  border-bottom: 1px solid #e5e7eb;
}

.mono {
  font-variant-numeric: tabular-nums;
  color: #64748b;
  font-weight: 700;
}

.empty-cell {
  text-align: center;
  color: #94a3b8;
}

.placeholder-row td {
  height: 44px;
}
</style>
