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
              <td>{{ sourceLabel(item.source) }}</td>
              <td>
                <span class="tag" :class="statusTone(item.status)">{{
                  statusLabel(item.status)
                }}</span>
              </td>
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
      try {
        await store.dispatch("superAdmin/ensureWorkTimeRecords", {
          force: true,
        });
        message.value = "等待操作";
        void nextTick(measureCapacity);
      } catch (error) {
        // 兜底：请求失败时优雅提示，避免未处理的 promise 异常触发整页错误。
        message.value = "考勤记录加载失败，请点击「刷新列表」重试";
        console.error("加载考勤记录失败：", error);
      }
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

    // 来源中文化：online_doctors 来自实时在线表，work_records 来自考勤补录表
    const sourceLabel = (source: string) => {
      if (source === "online_doctors") return "实时在线";
      if (source === "work_records") return "考勤记录";
      return source || "—";
    };

    // 状态中文化：online_doctors 源返回 online/offline，work_records 源已是中文
    const statusLabel = (status: string) => {
      if (status === "online") return "在线";
      if (status === "offline") return "离线";
      return status || "—";
    };

    // 状态着色：离线/异常类标红，便于一眼定位；在线/正常标绿，其余中性
    const statusTone = (status: string) => {
      if (
        ["offline", "离线", "迟到", "早退", "缺勤", "异常"].includes(status)
      ) {
        return "tag--danger";
      }
      if (["online", "在线", "正常"].includes(status)) {
        return "tag--ok";
      }
      return "tag--muted";
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
      sourceLabel,
      statusLabel,
      statusTone,
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

/* 状态胶囊：离线/异常红、在线/正常绿，便于一眼定位 */
.tag {
  display: inline-flex;
  align-items: center;
  padding: 3px 10px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.tag--danger {
  background: #fdeef0;
  color: #b04455;
}

.tag--ok {
  background: #e7f5ef;
  color: #247b62;
}

.tag--muted {
  background: #f1f5f9;
  color: #64748b;
}
</style>
