<template>
  <section class="attendance-mobile-page">
    <div class="phone-panel">
      <header class="top-bar">
        <div>
          <p>我的考勤</p>
          <h2>{{ monthLabel }}</h2>
        </div>
        <input v-model="month" class="month-field" type="month" @change="loadRecords" />
      </header>

      <section class="summary-grid">
        <article v-for="item in stats" :key="item.key">
          <span>{{ item.label }}</span>
          <strong>{{ item.value }}</strong>
        </article>
      </section>

      <section class="today-card">
        <div>
          <span>今日</span>
          <strong>{{ todayRecord ? statusLabel(todayRecord.status) : "暂无记录" }}</strong>
        </div>
        <div class="today-times">
          <span>上班 {{ timeText(todayRecord?.check_in_at || "") }}</span>
          <span>下班 {{ timeText(todayRecord?.check_out_at || "") }}</span>
        </div>
      </section>

      <div v-if="message" class="message">{{ message }}</div>

      <section class="record-list">
        <article v-for="item in records" :key="item.work_date" class="record-card">
          <div class="record-main">
            <div>
              <strong>{{ dayText(item.work_date) }}</strong>
              <span>{{ item.work_date }}</span>
            </div>
            <span class="tag" :class="statusTone(item.status)">
              {{ statusLabel(item.status) }}
            </span>
          </div>
          <div class="time-row">
            <span>上班 {{ timeText(item.check_in_at) }}</span>
            <span>下班 {{ timeText(item.check_out_at) }}</span>
          </div>
          <p v-if="item.is_corrected" class="correction">
            已修正：{{ item.correction_note || "管理员补录" }}
          </p>
        </article>

        <div v-if="!loading && records.length === 0" class="empty-card">
          当前月份暂无考勤记录
        </div>
      </section>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { attendanceApi } from "@/modules/user/api/userApi";
import {
  UserAttendanceRecord,
  UserAttendanceStatus,
} from "@/modules/user/api/types";

const statusOptions: Record<UserAttendanceStatus, string> = {
  normal: "正常",
  late: "迟到",
  early_leave: "早退",
  late_and_early: "迟到且早退",
  missing_out: "缺少下班卡",
  absent: "缺勤",
};

const localDate = () => {
  const value = new Date();
  value.setMinutes(value.getMinutes() - value.getTimezoneOffset());
  return value.toISOString().slice(0, 10);
};
const currentMonth = () => localDate().slice(0, 7);
const today = () => localDate();

const month = ref(currentMonth());
const records = ref<UserAttendanceRecord[]>([]);
const loading = ref(false);
const message = ref("");

const monthLabel = computed(() => month.value.replace("-", "年") + "月");

const todayRecord = computed(() =>
  records.value.find((item) => item.work_date === today())
);

const stats = computed(() => {
  const total = records.value.length;
  const normal = records.value.filter((item) => item.status === "normal").length;
  const abnormal = records.value.filter((item) => item.status !== "normal").length;
  const corrected = records.value.filter((item) => item.is_corrected).length;

  return [
    { key: "total", label: "记录", value: total },
    { key: "normal", label: "正常", value: normal },
    { key: "abnormal", label: "异常", value: abnormal },
    { key: "corrected", label: "修正", value: corrected },
  ];
});

const statusLabel = (status: UserAttendanceStatus | string) =>
  statusOptions[status as UserAttendanceStatus] || status || "-";

const statusTone = (status: UserAttendanceStatus | string) => {
  if (status === "normal") return "tag--ok";
  if (["late", "early_leave", "late_and_early"].includes(status)) {
    return "tag--warn";
  }
  return "tag--danger";
};

const timeText = (value: string) => {
  if (!value) return "-";
  const parts = value.replace("T", " ").split(" ");
  return parts[1]?.slice(0, 5) || value;
};

const dayText = (value: string) => {
  const date = new Date(`${value}T00:00:00`);
  if (Number.isNaN(date.getTime())) return "未知";
  return ["周日", "周一", "周二", "周三", "周四", "周五", "周六"][
    date.getDay()
  ];
};

const loadRecords = async () => {
  loading.value = true;
  message.value = "";
  try {
    records.value = await attendanceApi.getMyAttendance(month.value);
  } catch (error) {
    records.value = [];
    message.value = `考勤加载失败：${String((error as Error).message || error)}`;
  } finally {
    loading.value = false;
  }
};

onMounted(() => {
  void loadRecords();
});
</script>

<style scoped>
.attendance-mobile-page {
  min-height: 100%;
  padding: 22px;
  box-sizing: border-box;
  color: #1f3a36;
}

.phone-panel {
  width: min(100%, 520px);
  margin: 0 auto;
  display: grid;
  gap: 14px;
}

.top-bar {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.top-bar p {
  margin: 0 0 4px;
  color: #6b7d77;
  font-size: 13px;
  font-weight: 700;
}

.top-bar h2 {
  margin: 0;
  color: #1f3a36;
  font-size: 24px;
}

.month-field {
  min-height: 36px;
  border: 1px solid #dbe8e2;
  border-radius: 10px;
  background: #fff;
  color: #1f3a36;
  font: inherit;
  padding: 0 10px;
}

.summary-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 8px;
}

.summary-grid article,
.today-card,
.record-card,
.empty-card,
.message {
  border: 1px solid #e4eee8;
  border-radius: 12px;
  background: #fff;
  box-shadow: 0 1px 2px rgba(31, 58, 54, 0.04);
}

.summary-grid article {
  padding: 12px 10px;
  text-align: center;
}

.summary-grid span {
  display: block;
  color: #6b7d77;
  font-size: 12px;
  font-weight: 700;
}

.summary-grid strong {
  display: block;
  margin-top: 4px;
  color: #1f3a36;
  font-size: 20px;
}

.today-card {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 16px;
  background: linear-gradient(135deg, #2f9e8f, #1f7a6c);
  color: #fff;
}

.today-card span {
  display: block;
  opacity: 0.86;
  font-size: 12px;
  font-weight: 700;
}

.today-card strong {
  display: block;
  margin-top: 4px;
  font-size: 22px;
}

.today-times {
  display: grid;
  gap: 4px;
  text-align: right;
}

.message {
  padding: 12px;
  color: #b91c1c;
  font-size: 13px;
  font-weight: 700;
}

.record-list {
  display: grid;
  gap: 10px;
}

.record-card {
  padding: 14px;
}

.record-main,
.time-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.record-main strong {
  display: block;
  color: #1f3a36;
  font-size: 16px;
}

.record-main span,
.time-row span,
.correction {
  color: #6b7d77;
  font-size: 12px;
}

.time-row {
  margin-top: 12px;
  border-top: 1px solid #edf4f0;
  padding-top: 12px;
}

.correction {
  margin: 10px 0 0;
  color: #c2671b;
}

.tag {
  display: inline-flex;
  align-items: center;
  min-height: 24px;
  border-radius: 999px;
  padding: 0 9px;
  font-size: 12px;
  font-weight: 800;
}

.tag--ok {
  background: #e7f5f1;
  color: #1f7a6c;
}

.tag--warn {
  background: #fff7ed;
  color: #c2671b;
}

.tag--danger {
  background: #fee2e2;
  color: #b91c1c;
}

.empty-card {
  padding: 28px 14px;
  color: #94a3b8;
  text-align: center;
}

@media (max-width: 640px) {
  .attendance-mobile-page {
    padding: 16px;
  }

  .summary-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .today-card {
    align-items: flex-start;
    flex-direction: column;
  }

  .today-times {
    text-align: left;
  }
}
</style>
