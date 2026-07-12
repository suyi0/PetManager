<template>
  <section class="page attendance-page">
    <section v-if="!canReadAttendance" class="panel access-state">
      <h2>无法访问考勤管理</h2>
      <p>当前账号没有考勤读取权限。</p>
    </section>
    <template v-else>
    <header class="attendance-hero">
      <div>
        <p class="eyebrow">Attendance</p>
        <h2>考勤管理</h2>
        <p class="hero-copy">
          统一查看考勤机打卡、手动补录和日终缺勤生成结果。
        </p>
      </div>
      <div class="hero-actions">
        <button class="btn btn--ghost" type="button" @click="loadAll">
          刷新
        </button>
        <button v-if="canManageAttendance" class="btn" type="button" @click="showManualDialog = true">
          手动补录
        </button>
        <button v-if="canManageAttendance" class="btn btn--dark" type="button" @click="showCloseDialog = true">
          日终关账
        </button>
      </div>
    </header>

    <section class="filters panel">
      <label>
        <span>月份</span>
        <input v-model="filters.month" class="field" type="month" />
      </label>
      <label>
        <span>开始日期</span>
        <input v-model="filters.start_date" class="field" type="date" />
      </label>
      <label>
        <span>结束日期</span>
        <input v-model="filters.end_date" class="field" type="date" />
      </label>
      <label>
        <span>员工ID</span>
        <input
          v-model.number="filters.user_id"
          class="field"
          min="1"
          placeholder="全部员工"
          type="number"
        />
      </label>
      <label>
        <span>状态</span>
        <select v-model="filters.status" class="field">
          <option value="">全部状态</option>
          <option v-for="item in statusOptions" :key="item.value" :value="item.value">
            {{ item.label }}
          </option>
        </select>
      </label>
      <button class="btn filter-submit" type="button" @click="loadRecords">
        查询
      </button>
    </section>

    <section class="stats-grid">
      <article v-for="item in stats" :key="item.key" class="stat-card">
        <span>{{ item.label }}</span>
        <strong>{{ item.value }}</strong>
      </article>
    </section>

    <section
      class="content-grid"
      :class="{ 'content-grid--records-only': !canManageAttendance }"
    >
      <article class="panel records-panel">
        <div class="panel-head">
          <div>
            <h3>考勤记录</h3>
            <p>{{ recordsMeta.loading ? "正在加载..." : `共 ${recordTotalText} 条` }}</p>
          </div>
          <span v-if="message" class="message">{{ message }}</span>
        </div>

        <div class="table-shell">
          <table>
            <thead>
              <tr>
                <th>员工</th>
                <th>部门</th>
                <th>日期</th>
                <th>上班打卡</th>
                <th>下班打卡</th>
                <th>状态</th>
                <th>修正</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="item in records" :key="`${item.user_id}-${item.work_date}`">
                <td>
                  <b>{{ item.name || "未命名员工" }}</b>
                  <small>#{{ item.user_id }}</small>
                </td>
                <td>{{ item.department_name || "未分配" }}</td>
                <td>{{ item.work_date || "-" }}</td>
                <td>{{ timeText(item.check_in_at) }}</td>
                <td>{{ timeText(item.check_out_at) }}</td>
                <td>
                  <span class="tag" :class="statusTone(item.status)">
                    {{ statusLabel(item.status) }}
                  </span>
                </td>
                <td>
                  <span v-if="item.is_corrected" class="tag tag--warn">
                    已修正
                  </span>
                  <span v-else class="muted">-</span>
                </td>
              </tr>
              <tr v-if="!recordsMeta.loading && records.length === 0">
                <td class="empty-cell" colspan="7">暂无符合条件的考勤记录</td>
              </tr>
            </tbody>
          </table>
        </div>
      </article>

      <aside v-if="canManageAttendance" class="panel devices-panel">
        <div class="panel-head">
          <div>
            <h3>考勤机</h3>
            <p>{{ devicesMeta.loading ? "正在加载..." : `${devices.length} 台设备` }}</p>
          </div>
          <button class="mini-btn" type="button" @click="showDeviceDialog = true">
            新增
          </button>
        </div>

        <div v-if="secretNotice" class="secret-box">
          <span>一次性密钥</span>
          <strong>{{ secretNotice }}</strong>
          <small>请立即配置到考勤机，关闭后不再展示。</small>
          <button
            class="secret-close"
            title="关闭一次性密钥"
            type="button"
            @click="secretNotice = ''"
          >
            ×
          </button>
        </div>

        <div class="device-list">
          <article v-for="device in devices" :key="device.id" class="device-card">
            <div>
              <b>{{ device.name }}</b>
              <small>{{ device.device_key }}</small>
            </div>
            <p>{{ device.vendor || "未知厂商" }} · {{ device.location || "未设置位置" }}</p>
            <p>最近心跳：{{ dateTimeText(device.last_seen_at) }}</p>
            <div class="device-actions">
              <span class="tag" :class="device.is_active ? 'tag--ok' : 'tag--muted'">
                {{ device.is_active ? "启用" : "停用" }}
              </span>
              <button class="link-button" type="button" @click="rotateSecret(device.id)">
                轮换密钥
              </button>
              <button
                class="link-button danger"
                :disabled="!device.is_active"
                type="button"
                @click="disableDevice(device.id)"
              >
                停用
              </button>
            </div>
          </article>
          <div v-if="!devicesMeta.loading && devices.length === 0" class="empty-card">
            暂无考勤机
          </div>
        </div>
      </aside>
    </section>

    <div v-if="showManualDialog" class="modal-backdrop" @click.self="showManualDialog = false">
      <form class="modal" @submit.prevent="submitManualPunch">
        <h3>手动补录</h3>
        <label>
          <span>员工ID</span>
          <input v-model.number="manualForm.user_id" class="field" min="1" required type="number" />
        </label>
        <label>
          <span>打卡时间</span>
          <input v-model="manualForm.punched_at" class="field" required type="datetime-local" />
        </label>
        <label>
          <span>原因</span>
          <textarea v-model.trim="manualForm.reason" class="field" required rows="3" />
        </label>
        <div class="modal-actions">
          <button class="btn btn--ghost" type="button" @click="showManualDialog = false">
            取消
          </button>
          <button class="btn" :disabled="submitting" type="submit">保存</button>
        </div>
      </form>
    </div>

    <div v-if="showCloseDialog" class="modal-backdrop" @click.self="showCloseDialog = false">
      <form class="modal" @submit.prevent="submitCloseDay">
        <h3>日终关账</h3>
        <p class="modal-copy">为指定日期生成缺勤记录，适合每日考勤机数据同步完成后执行。</p>
        <label>
          <span>关账日期</span>
          <input v-model="closeDate" class="field" required type="date" />
        </label>
        <div class="modal-actions">
          <button class="btn btn--ghost" type="button" @click="showCloseDialog = false">
            取消
          </button>
          <button class="btn btn--dark" :disabled="submitting" type="submit">
            执行关账
          </button>
        </div>
      </form>
    </div>

    <div v-if="showDeviceDialog" class="modal-backdrop" @click.self="showDeviceDialog = false">
      <form class="modal" @submit.prevent="submitDevice">
        <h3>新增考勤机</h3>
        <label>
          <span>设备名称</span>
          <input v-model.trim="deviceForm.name" class="field" required />
        </label>
        <label>
          <span>设备编码</span>
          <input v-model.trim="deviceForm.device_key" class="field" placeholder="留空自动生成" />
        </label>
        <label>
          <span>厂商</span>
          <input v-model.trim="deviceForm.vendor" class="field" placeholder="ZKTeco / Hikvision" />
        </label>
        <label>
          <span>位置</span>
          <input v-model.trim="deviceForm.location" class="field" placeholder="前台 / 员工入口" />
        </label>
        <div class="modal-actions">
          <button class="btn btn--ghost" type="button" @click="showDeviceDialog = false">
            取消
          </button>
          <button class="btn" :disabled="submitting" type="submit">创建</button>
        </div>
      </form>
    </div>
    </template>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, reactive, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import {
  AttendanceRecordQuery,
  AttendanceStatus,
} from "@/modules/super-admin/api/types";

const statusOptions: Array<{ value: AttendanceStatus; label: string }> = [
  { value: "normal", label: "正常" },
  { value: "late", label: "迟到" },
  { value: "early_leave", label: "早退" },
  { value: "late_and_early", label: "迟到且早退" },
  { value: "missing_out", label: "缺少下班卡" },
  { value: "absent", label: "缺勤" },
];

const localDate = () => {
  const value = new Date();
  value.setMinutes(value.getMinutes() - value.getTimezoneOffset());
  return value.toISOString().slice(0, 10);
};
const today = () => localDate();
const currentMonth = () => localDate().slice(0, 7);
const nowLocal = () => new Date(Date.now() - new Date().getTimezoneOffset() * 60000)
  .toISOString()
  .slice(0, 16);

export default defineComponent({
  name: "SuperAdminWorktime",
  setup() {
    const store = useStore(storeKey);
    const canReadAttendance = computed(() =>
      store.state.auth.permissions.includes("attendance:read")
    );
    const canManageAttendance = computed(() =>
      store.state.auth.permissions.includes("attendance:manage")
    );
    const message = ref("");
    const submitting = ref(false);
    const secretNotice = ref("");
    const showManualDialog = ref(false);
    const showCloseDialog = ref(false);
    const showDeviceDialog = ref(false);
    const closeDate = ref(today());

    const filters = reactive<AttendanceRecordQuery>({
      month: currentMonth(),
      start_date: "",
      end_date: "",
      status: "",
    });

    const manualForm = reactive({
      user_id: null as number | null,
      punched_at: nowLocal(),
      reason: "",
    });

    const deviceForm = reactive({
      name: "",
      device_key: "",
      vendor: "",
      location: "",
    });

    const records = computed(() => store.state.superAdmin.attendanceRecords);
    const devices = computed(() => store.state.superAdmin.attendanceDevices);
    const recordsMeta = computed(() => store.state.superAdmin.attendanceRecordsMeta);
    const devicesMeta = computed(() => store.state.superAdmin.attendanceDevicesMeta);
    const recordTotalText = computed(() =>
      store.state.superAdmin.attendanceRecordTotal || records.value.length
    );

    const query = (): AttendanceRecordQuery => ({
      month: filters.start_date || filters.end_date ? undefined : filters.month,
      start_date: filters.start_date || undefined,
      end_date: filters.end_date || undefined,
      user_id: filters.user_id || undefined,
      status: filters.status || undefined,
    });

    const loadRecords = async () => {
      message.value = "";
      try {
        await store.dispatch("superAdmin/fetchAttendanceRecords", query());
      } catch (error) {
        message.value = `考勤记录加载失败：${String((error as Error).message || error)}`;
      }
    };

    const loadAll = async () => {
      if (!canReadAttendance.value) return;

      const tasks: Promise<unknown>[] = [loadRecords()];
      if (canManageAttendance.value) {
        tasks.push(
          store.dispatch("superAdmin/ensureAttendanceDevices", { force: true })
        );
      }
      await Promise.all(tasks);
    };

    const stats = computed(() => {
      const rows = records.value;
      const abnormal = rows.filter((item) => item.status !== "normal").length;
      const corrected = rows.filter((item) => item.is_corrected).length;
      const missing = rows.filter((item) =>
        ["missing_out", "absent"].includes(item.status)
      ).length;

      return [
        { key: "total", label: "记录", value: rows.length },
        { key: "abnormal", label: "异常", value: abnormal },
        { key: "corrected", label: "已修正", value: corrected },
        { key: "missing", label: "缺卡/缺勤", value: missing },
      ];
    });

    const statusLabel = (status: AttendanceStatus | string) =>
      statusOptions.find((item) => item.value === status)?.label || status || "-";

    const statusTone = (status: AttendanceStatus | string) => {
      if (status === "normal") return "tag--ok";
      if (["late", "early_leave", "late_and_early"].includes(status)) return "tag--warn";
      return "tag--danger";
    };

    const timeText = (value: string) => {
      if (!value) return "-";
      const parts = value.replace("T", " ").split(" ");
      return parts[1]?.slice(0, 5) || value;
    };

    const dateTimeText = (value: string) => {
      if (!value) return "从未";
      return value.replace("T", " ").slice(0, 16);
    };

    const submitManualPunch = async () => {
      if (!manualForm.user_id) return;

      submitting.value = true;
      try {
        await store.dispatch("superAdmin/manualAttendancePunch", {
          user_id: manualForm.user_id,
          punched_at: manualForm.punched_at.replace("T", " "),
          verify_mode: "manual",
          reason: manualForm.reason,
          refreshQuery: query(),
        });
        message.value = "补录成功";
        showManualDialog.value = false;
      } catch (error) {
        message.value = `补录失败：${String((error as Error).message || error)}`;
      } finally {
        submitting.value = false;
      }
    };

    const submitCloseDay = async () => {
      submitting.value = true;
      try {
        const created = await store.dispatch("superAdmin/closeAttendanceDay", {
          work_date: closeDate.value,
          refreshQuery: query(),
        });
        message.value = `关账完成，生成 ${created} 条缺勤记录`;
        showCloseDialog.value = false;
      } catch (error) {
        message.value = `关账失败：${String((error as Error).message || error)}`;
      } finally {
        submitting.value = false;
      }
    };

    const submitDevice = async () => {
      submitting.value = true;
      try {
        const result = await store.dispatch("superAdmin/createAttendanceDevice", {
          name: deviceForm.name,
          device_key: deviceForm.device_key || undefined,
          vendor: deviceForm.vendor || undefined,
          location: deviceForm.location || undefined,
        });
        secretNotice.value = result.secret;
        Object.assign(deviceForm, {
          name: "",
          device_key: "",
          vendor: "",
          location: "",
        });
        showDeviceDialog.value = false;
      } catch (error) {
        message.value = `创建设备失败：${String((error as Error).message || error)}`;
      } finally {
        submitting.value = false;
      }
    };

    const rotateSecret = async (deviceId: number) => {
      if (!window.confirm("轮换后旧密钥会立即失效，确认继续吗？")) return;

      try {
        const result = await store.dispatch(
          "superAdmin/rotateAttendanceDeviceSecret",
          deviceId
        );
        secretNotice.value = result.secret;
      } catch (error) {
        message.value = `轮换密钥失败：${String((error as Error).message || error)}`;
      }
    };

    const disableDevice = async (deviceId: number) => {
      if (!window.confirm("停用后该设备将无法继续上报考勤，确认停用吗？")) return;

      try {
        await store.dispatch("superAdmin/disableAttendanceDevice", deviceId);
        message.value = "设备已停用";
      } catch (error) {
        message.value = `停用设备失败：${String((error as Error).message || error)}`;
      }
    };

    onMounted(() => {
      void loadAll();
    });

    return {
      closeDate,
      canManageAttendance,
      canReadAttendance,
      devices,
      devicesMeta,
      deviceForm,
      filters,
      manualForm,
      message,
      records,
      recordsMeta,
      recordTotalText,
      secretNotice,
      showCloseDialog,
      showDeviceDialog,
      showManualDialog,
      stats,
      statusOptions,
      submitting,
      dateTimeText,
      disableDevice,
      loadAll,
      loadRecords,
      rotateSecret,
      statusLabel,
      statusTone,
      submitCloseDay,
      submitDevice,
      submitManualPunch,
      timeText,
    };
  },
});
</script>

<style scoped>
.attendance-page {
  display: grid;
  grid-template-rows: auto auto auto minmax(0, 1fr);
  gap: 14px;
  height: 100%;
  min-height: 0;
}

.attendance-hero,
.panel,
.stat-card {
  border: 1px solid #e7e9ee;
  border-radius: 12px;
  background: #fff;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

.access-state {
  align-self: start;
  padding: 28px;
}

.access-state h2,
.access-state p {
  margin: 0;
}

.access-state p {
  margin-top: 8px;
  color: #64748b;
}

.attendance-hero {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 18px 20px;
}

.eyebrow {
  margin: 0 0 4px;
  color: #4f97e0;
  font-size: 12px;
  font-weight: 800;
  text-transform: uppercase;
}

.attendance-hero h2,
.panel-head h3,
.modal h3 {
  margin: 0;
  color: #0f172a;
}

.attendance-hero h2 {
  font-size: 22px;
}

.hero-copy,
.panel-head p,
.modal-copy,
.muted {
  margin: 4px 0 0;
  color: #64748b;
  font-size: 13px;
}

.hero-actions,
.modal-actions,
.device-actions {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.btn,
.mini-btn {
  border: 0;
  border-radius: 9px;
  background: #4f97e0;
  color: #fff;
  cursor: pointer;
  font-weight: 700;
}

.btn {
  min-height: 38px;
  padding: 0 14px;
}

.mini-btn {
  min-height: 32px;
  padding: 0 12px;
}

.btn--ghost {
  border: 1px solid #d8dee8;
  background: #fff;
  color: #334155;
}

.btn--dark {
  background: #1f2937;
}

.btn:disabled,
.link-button:disabled {
  cursor: not-allowed;
  opacity: 0.45;
}

.filters {
  display: grid;
  grid-template-columns: repeat(5, minmax(120px, 1fr)) auto;
  gap: 10px;
  padding: 14px;
  align-items: end;
}

label {
  display: grid;
  gap: 6px;
  color: #64748b;
  font-size: 12px;
  font-weight: 700;
}

.field {
  width: 100%;
  min-height: 38px;
  border: 1px solid #d8dee8;
  border-radius: 9px;
  background: #fff;
  color: #0f172a;
  font: inherit;
  padding: 0 10px;
  box-sizing: border-box;
}

textarea.field {
  padding: 10px;
  resize: vertical;
}

.filter-submit {
  align-self: end;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
}

.stat-card {
  padding: 14px 16px;
}

.stat-card span {
  color: #64748b;
  font-size: 13px;
  font-weight: 700;
}

.stat-card strong {
  display: block;
  margin-top: 4px;
  color: #0f172a;
  font-size: 24px;
}

.content-grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 320px;
  gap: 14px;
  min-height: 0;
}

.content-grid--records-only {
  grid-template-columns: minmax(0, 1fr);
}

.records-panel,
.devices-panel {
  min-height: 0;
  overflow: hidden;
}

.panel-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 14px 16px;
  border-bottom: 1px solid #edf0f5;
}

.message {
  color: #4f97e0;
  font-size: 13px;
  font-weight: 700;
}

.table-shell {
  height: calc(100% - 69px);
  min-height: 260px;
  overflow: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

th,
td {
  padding: 12px 14px;
  border-bottom: 1px solid #edf0f5;
  text-align: left;
  white-space: nowrap;
}

th {
  position: sticky;
  top: 0;
  z-index: 1;
  background: #f8fafc;
  color: #64748b;
  font-size: 12px;
}

td b,
.device-card b {
  display: block;
  color: #0f172a;
}

td small,
.device-card small,
.secret-box small {
  display: block;
  color: #94a3b8;
  font-size: 12px;
}

.secret-box {
  position: relative;
}

.secret-close {
  position: absolute;
  top: 8px;
  right: 8px;
  width: 28px;
  height: 28px;
  border: 0;
  background: transparent;
  color: #64748b;
  cursor: pointer;
  font-size: 20px;
}

.empty-cell,
.empty-card {
  color: #94a3b8;
  text-align: center;
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
  color: #2f7dd0;
}

.tag--warn {
  background: #fff7ed;
  color: #c2671b;
}

.tag--danger {
  background: #fee2e2;
  color: #b91c1c;
}

.tag--muted {
  background: #f1f5f9;
  color: #64748b;
}

.link-button {
  border: 0;
  background: transparent;
  color: #4f97e0;
  cursor: pointer;
  font: inherit;
  font-weight: 800;
  padding: 0;
}

.link-button.danger {
  color: #b91c1c;
}

.device-list {
  display: grid;
  gap: 10px;
  padding: 14px;
  max-height: calc(100% - 69px);
  overflow: auto;
}

.device-card,
.secret-box,
.empty-card {
  border: 1px solid #edf0f5;
  border-radius: 10px;
  padding: 12px;
}

.device-card p {
  margin: 8px 0 0;
  color: #64748b;
  font-size: 12px;
}

.device-actions {
  margin-top: 10px;
}

.secret-box {
  margin: 14px 14px 0;
  background: #f8fafc;
}

.secret-box span {
  color: #64748b;
  font-size: 12px;
  font-weight: 800;
}

.secret-box strong {
  display: block;
  margin: 6px 0;
  color: #0f172a;
  word-break: break-all;
}

.modal-backdrop {
  position: fixed;
  inset: 0;
  z-index: 50;
  display: grid;
  place-items: center;
  background: rgba(15, 23, 42, 0.32);
  padding: 18px;
}

.modal {
  width: min(420px, 100%);
  display: grid;
  gap: 14px;
  border-radius: 14px;
  background: #fff;
  padding: 18px;
  box-shadow: 0 24px 60px rgba(15, 23, 42, 0.18);
}

.modal-actions {
  justify-content: flex-end;
}

@media (max-width: 1180px) {
  .filters {
    grid-template-columns: repeat(3, minmax(120px, 1fr));
  }

  .content-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 760px) {
  .attendance-page {
    height: auto;
  }

  .attendance-hero,
  .panel-head {
    align-items: stretch;
    flex-direction: column;
  }

  .filters,
  .stats-grid {
    grid-template-columns: 1fr;
  }

  .hero-actions,
  .modal-actions {
    width: 100%;
  }

  .btn {
    flex: 1;
  }
}
</style>
