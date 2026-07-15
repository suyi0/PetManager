<template>
  <section class="approval-page">
    <header class="page-head">
      <div>
        <span class="eyebrow">MANAGEMENT DECISIONS</span>
        <h1>任职与薪酬审批</h1>
        <p>任职决定与薪资承诺分开审批；批准薪酬不会直接写入生效工资配置。</p>
      </div>
      <button
        type="button"
        class="refresh"
        :disabled="loading"
        @click="loadQueues"
      >
        刷新队列
      </button>
    </header>

    <div class="approval-shell">
      <aside class="queue-panel panel">
        <nav class="queue-tabs">
          <button
            type="button"
            :class="{ active: activeQueue === 'employment' }"
            @click="activeQueue = 'employment'"
          >
            任职申请 <span>{{ employmentApprovals.length }}</span>
          </button>
          <button
            type="button"
            :class="{ active: activeQueue === 'compensation' }"
            @click="activeQueue = 'compensation'"
          >
            薪酬提案 <span>{{ compensationApprovals.length }}</span>
          </button>
        </nav>

        <div class="queue-list">
          <template v-if="activeQueue === 'employment'">
            <button
              v-for="item in employmentApprovals"
              :key="item.id"
              type="button"
              class="queue-row"
              :class="{ selected: selectedEmployment?.id === item.id }"
              @click="selectEmployment(item)"
            >
              <span class="queue-index">{{ actionIcon(item.action) }}</span>
              <span class="queue-copy">
                <strong>{{ item.userName || `职工 #${item.userId}` }}</strong>
                <small
                  >{{ actionLabel(item.action) }} ·
                  {{
                    item.toPositionName || `职位 #${item.toPositionId || "-"}`
                  }}</small
                >
              </span>
              <span class="queue-date">{{ item.effectiveFrom }}</span>
            </button>
          </template>
          <template v-else>
            <button
              v-for="item in compensationApprovals"
              :key="item.id"
              type="button"
              class="queue-row"
              :class="{ selected: selectedCompensation?.id === item.id }"
              @click="selectCompensation(item)"
            >
              <span class="queue-index money">¥</span>
              <span class="queue-copy">
                <strong>{{ item.userName || `职工 #${item.userId}` }}</strong>
                <small
                  >{{ phaseLabel(item.phase) }} ·
                  {{ item.positionName || `任职 #${item.employmentId}` }}</small
                >
              </span>
              <span class="queue-date">{{ item.effectiveFrom }}</span>
            </button>
          </template>

          <div v-if="currentItemsEmpty" class="empty">
            <span>✓</span>
            <strong>当前队列已处理完毕</strong>
            <small>新申请提交后会自动进入对应队列。</small>
          </div>
        </div>
      </aside>

      <main class="decision-panel panel">
        <template v-if="activeQueue === 'employment' && selectedEmployment">
          <section class="decision-head">
            <div>
              <span class="eyebrow"
                >EMPLOYMENT #{{ selectedEmployment.id }}</span
              >
              <h2>
                {{
                  selectedEmployment.userName ||
                  `职工 #${selectedEmployment.userId}`
                }}
              </h2>
              <p>
                {{ actionLabel(selectedEmployment.action) }}申请 · row v{{
                  selectedEmployment.rowVersion
                }}
              </p>
            </div>
            <span class="status">待任职审批</span>
          </section>

          <div class="decision-body">
            <section class="route-card">
              <article>
                <small>当前职位</small
                ><strong>{{
                  selectedEmployment.fromPositionName ||
                  `#${selectedEmployment.fromPositionId || "无"}`
                }}</strong>
              </article>
              <span>→</span>
              <article>
                <small>目标职位</small
                ><strong>{{
                  selectedEmployment.toPositionName ||
                  `#${selectedEmployment.toPositionId || "离职"}`
                }}</strong>
              </article>
            </section>
            <section class="facts">
              <article>
                <small>组织快照</small
                ><strong
                  >{{
                    selectedEmployment.branchName ||
                    `分院 #${selectedEmployment.branchId}`
                  }}
                  ·
                  {{
                    selectedEmployment.departmentName ||
                    `部门 #${selectedEmployment.departmentId}`
                  }}</strong
                >
              </article>
              <article>
                <small>计划生效</small
                ><strong>{{ selectedEmployment.effectiveFrom }}</strong>
              </article>
              <article>
                <small>当前任职状态</small
                ><strong>{{
                  employmentStatusLabel(selectedEmployment.employmentStatus)
                }}</strong>
              </article>
              <article>
                <small>申请人</small
                ><strong>#{{ selectedEmployment.requestedBy || "-" }}</strong>
              </article>
            </section>
            <section class="reason-card">
              <small>申请依据</small>
              <p>{{ selectedEmployment.reason || "未填写申请依据" }}</p>
            </section>
          </div>
        </template>

        <template
          v-else-if="activeQueue === 'compensation' && selectedCompensation"
        >
          <section class="decision-head">
            <div>
              <span class="eyebrow"
                >COMPENSATION #{{ selectedCompensation.id }}</span
              >
              <h2>
                {{
                  selectedCompensation.userName ||
                  `职工 #${selectedCompensation.userId}`
                }}
              </h2>
              <p>
                {{ phaseLabel(selectedCompensation.phase) }} · row v{{
                  selectedCompensation.rowVersion
                }}
              </p>
            </div>
            <span class="status salary">待薪酬批准</span>
          </section>

          <div class="decision-body">
            <section class="salary-card">
              <small>{{
                selectedCompensation.payType === "hourly"
                  ? "小时薪资"
                  : "基础月薪"
              }}</small>
              <strong
                >{{
                  money(
                    selectedCompensation.payType === "hourly"
                      ? selectedCompensation.hourlyRate
                      : selectedCompensation.baseSalary
                  )
                }}<em>{{
                  selectedCompensation.payType === "hourly" ? "/小时" : "/月"
                }}</em></strong
              >
              <span
                >五险一金
                {{
                  money(selectedCompensation.socialInsuranceHousingFund)
                }}</span
              >
            </section>
            <section class="facts">
              <article>
                <small>部门职位</small
                ><strong
                  >{{
                    selectedCompensation.departmentName ||
                    `部门 #${selectedCompensation.departmentId}`
                  }}
                  ·
                  {{
                    selectedCompensation.positionName || "任职待同步"
                  }}</strong
                >
              </article>
              <article>
                <small>计划生效</small
                ><strong>{{ selectedCompensation.effectiveFrom }}</strong>
              </article>
              <article>
                <small>案件受理人</small
                ><strong>#{{ selectedCompensation.assigneeUserId }}</strong>
              </article>
              <article>
                <small>任职版本快照</small
                ><strong>{{
                  selectedCompensation.expectedEmploymentRowVersion
                    ? `v${selectedCompensation.expectedEmploymentRowVersion}`
                    : "批准时固化"
                }}</strong>
              </article>
            </section>
            <section class="reason-card">
              <small>提案说明</small>
              <p>{{ selectedCompensation.note || "未填写提案说明" }}</p>
            </section>
            <p class="sod-note">
              职责分离：拟案人不能批准自己的提案；批准后仍需由另一名财务人员激活。
            </p>
          </div>
        </template>

        <div v-else class="empty detail-empty">
          <span>02</span>
          <strong>选择一条待办</strong>
          <small>左侧选择后查看组织快照、业务依据与版本信息。</small>
        </div>

        <footer v-if="hasSelection" class="decision-actions">
          <label>
            <span>审批意见（退回 / 拒绝时必填）</span>
            <textarea
              v-model.trim="decisionReason"
              placeholder="说明批准依据或需要补充的材料"
            />
          </label>
          <div>
            <button
              type="button"
              class="reject"
              :disabled="saving || !decisionReason"
              @click="rejectCurrent"
            >
              {{ activeQueue === "employment" ? "拒绝申请" : "退回提案" }}
            </button>
            <button
              type="button"
              class="approve"
              :disabled="saving"
              @click="approveCurrent"
            >
              批准
            </button>
          </div>
        </footer>
      </main>
    </div>

    <p v-if="feedback" class="feedback" :class="feedbackType">{{ feedback }}</p>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref, watch } from "vue";
import { getHttpErrorMessage } from "@/api/httpError";
import {
  AssignmentApproval,
  CompensationProposal,
} from "@/shared/types/employmentWorkflow";
import { bossApi } from "../../api/bossApi";

export default defineComponent({
  name: "BossApprovals",
  setup() {
    const activeQueue = ref<"employment" | "compensation">("employment");
    const employmentApprovals = ref<AssignmentApproval[]>([]);
    const compensationApprovals = ref<CompensationProposal[]>([]);
    const selectedEmployment = ref<AssignmentApproval | null>(null);
    const selectedCompensation = ref<CompensationProposal | null>(null);
    const decisionReason = ref("");
    const loading = ref(false);
    const saving = ref(false);
    const feedback = ref("");
    const feedbackType = ref<"success" | "error">("success");

    const currentItemsEmpty = computed(() =>
      activeQueue.value === "employment"
        ? employmentApprovals.value.length === 0
        : compensationApprovals.value.length === 0
    );
    const hasSelection = computed(() =>
      activeQueue.value === "employment"
        ? Boolean(selectedEmployment.value)
        : Boolean(selectedCompensation.value)
    );

    const loadQueues = async () => {
      loading.value = true;
      feedback.value = "";
      try {
        const [employment, compensation] = await Promise.all([
          bossApi.listEmploymentApprovals(),
          bossApi.listCompensationApprovals(),
        ]);
        employmentApprovals.value = employment.items;
        compensationApprovals.value = compensation.items;
        if (selectedEmployment.value) {
          selectedEmployment.value =
            employment.items.find(
              (item) => item.id === selectedEmployment.value?.id
            ) ?? null;
        }
        if (selectedCompensation.value) {
          selectedCompensation.value =
            compensation.items.find(
              (item) => item.id === selectedCompensation.value?.id
            ) ?? null;
        }
      } catch (error) {
        feedback.value = getHttpErrorMessage(error, "审批队列加载失败");
        feedbackType.value = "error";
      } finally {
        loading.value = false;
      }
    };
    const selectEmployment = (item: AssignmentApproval) => {
      selectedEmployment.value = item;
      decisionReason.value = "";
    };
    const selectCompensation = (item: CompensationProposal) => {
      selectedCompensation.value = item;
      decisionReason.value = "";
    };
    const decide = async (approve: boolean) => {
      saving.value = true;
      feedback.value = "";
      try {
        if (activeQueue.value === "employment" && selectedEmployment.value) {
          await bossApi.decideEmploymentApproval(selectedEmployment.value.id, {
            action: approve ? "approve" : "reject",
            reason: decisionReason.value || "批准任职申请",
            expectedRowVersion: selectedEmployment.value.rowVersion,
          });
        } else if (selectedCompensation.value) {
          await bossApi.decideCompensationApproval(
            selectedCompensation.value.id,
            {
              action: approve ? "approve" : "return",
              reason: decisionReason.value || "批准薪酬提案",
              expectedRowVersion: selectedCompensation.value.rowVersion,
            }
          );
        }
        feedback.value = approve
          ? "审批已批准，队列已刷新"
          : "申请已退回，队列已刷新";
        feedbackType.value = "success";
        decisionReason.value = "";
        await loadQueues();
      } catch (error) {
        feedback.value = getHttpErrorMessage(error, "审批操作失败");
        feedbackType.value = "error";
      } finally {
        saving.value = false;
      }
    };
    const approveCurrent = () => void decide(true);
    const rejectCurrent = () => void decide(false);
    const actionLabel = (action: string) =>
      ((
        {
          onboard: "入职",
          transfer: "调岗",
          regularize: "转正",
          offboard: "离职",
        } as Record<string, string>
      )[action] || action);
    const actionIcon = (action: string) =>
      ((
        {
          onboard: "+",
          transfer: "↔",
          regularize: "✓",
          offboard: "−",
        } as Record<string, string>
      )[action] || "·");
    const phaseLabel = (phase: string) =>
      ((
        {
          probation: "试用期薪酬",
          regular: "正式薪酬",
          adjustment: "在职调薪",
        } as Record<string, string>
      )[phase] || phase);
    const employmentStatusLabel = (status: string) =>
      ((
        {
          onboarding: "入职处理中",
          probation: "试用期",
          regularization_pending: "转正待薪酬",
          active: "正式在职",
          separated: "已离职",
        } as Record<string, string>
      )[status] || status);
    const money = (value?: number | null) =>
      `￥${Number(value ?? 0).toLocaleString("zh-CN", {
        minimumFractionDigits: 2,
        maximumFractionDigits: 2,
      })}`;

    watch(activeQueue, () => {
      decisionReason.value = "";
      feedback.value = "";
    });
    onMounted(() => void loadQueues());

    return {
      actionIcon,
      actionLabel,
      activeQueue,
      approveCurrent,
      compensationApprovals,
      currentItemsEmpty,
      decisionReason,
      employmentApprovals,
      employmentStatusLabel,
      feedback,
      feedbackType,
      hasSelection,
      loadQueues,
      loading,
      money,
      phaseLabel,
      rejectCurrent,
      saving,
      selectCompensation,
      selectEmployment,
      selectedCompensation,
      selectedEmployment,
    };
  },
});
</script>

<style scoped>
.approval-page {
  --ink: #12243a;
  --muted: #71849a;
  --line: #dbe6f1;
  --blue: #2f7fca;
  --green: #168364;
  --red: #c95252;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 14px;
  height: 100%;
  min-height: 0;
  color: var(--ink);
}
button,
textarea {
  font: inherit;
}
h1,
h2,
p {
  margin: 0;
}
.page-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  min-height: 70px;
  gap: 20px;
}
.eyebrow {
  color: #7c91a8;
  font-size: 10px;
  font-weight: 900;
  letter-spacing: 0.16em;
}
.page-head h1 {
  margin-top: 4px;
  font-size: 25px;
  letter-spacing: -0.03em;
}
.page-head p {
  margin-top: 5px;
  color: var(--muted);
  font-size: 12px;
}
.refresh {
  height: 39px;
  padding: 0 15px;
  border: 1px solid var(--line);
  border-radius: 9px;
  background: #fff;
  color: #435a72;
  font-size: 11px;
  font-weight: 800;
  cursor: pointer;
}
.approval-shell {
  display: grid;
  grid-template-columns: minmax(300px, 0.72fr) minmax(600px, 1.4fr);
  gap: 14px;
  min-height: 0;
}
.panel {
  min-height: 0;
  border: 1px solid var(--line);
  border-radius: 16px;
  background: #fff;
  box-shadow: 0 2px 8px rgba(31, 60, 91, 0.04);
  overflow: hidden;
}
.queue-panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
}
.queue-tabs {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 4px;
  padding: 8px;
  border-bottom: 1px solid var(--line);
}
.queue-tabs button {
  height: 42px;
  border: 0;
  border-radius: 9px;
  background: transparent;
  color: var(--muted);
  font-size: 11px;
  font-weight: 800;
  cursor: pointer;
}
.queue-tabs button.active {
  background: #edf5ff;
  color: var(--blue);
}
.queue-tabs span {
  margin-left: 4px;
  padding: 2px 6px;
  border-radius: 999px;
  background: #e8eef5;
  font-size: 9px;
}
.queue-list {
  min-height: 0;
  overflow: auto;
  padding: 8px;
}
.queue-row {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  align-items: center;
  gap: 11px;
  width: 100%;
  min-height: 68px;
  padding: 9px 10px;
  border: 1px solid transparent;
  border-radius: 11px;
  background: transparent;
  color: var(--ink);
  text-align: left;
  cursor: pointer;
}
.queue-row:hover {
  background: #f7faff;
}
.queue-row.selected {
  border-color: #cfe2f6;
  background: #eef6ff;
}
.queue-index {
  display: grid;
  place-items: center;
  width: 32px;
  height: 32px;
  border-radius: 9px;
  background: #edf5ff;
  color: var(--blue);
  font-weight: 900;
}
.queue-index.money {
  background: #edf8f4;
  color: var(--green);
}
.queue-copy {
  display: grid;
  gap: 4px;
  min-width: 0;
}
.queue-copy strong {
  overflow: hidden;
  font-size: 12px;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.queue-copy small,
.queue-date {
  color: var(--muted);
  font-size: 9px;
}
.decision-panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr) auto;
}
.decision-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 15px;
  padding: 19px 22px;
  border-bottom: 1px solid var(--line);
}
.decision-head h2 {
  margin-top: 4px;
  font-size: 20px;
}
.decision-head p {
  margin-top: 5px;
  color: var(--muted);
  font-size: 10px;
}
.status {
  padding: 6px 9px;
  border: 1px solid #efd9ae;
  border-radius: 7px;
  background: #fff7e8;
  color: #b57215;
  font-size: 10px;
  font-weight: 900;
}
.status.salary {
  border-color: #cfe1f5;
  background: #edf5ff;
  color: var(--blue);
}
.decision-body {
  min-height: 0;
  overflow: auto;
  padding: 20px 22px;
  background: #fbfcfe;
}
.route-card {
  display: grid;
  grid-template-columns: 1fr auto 1fr;
  align-items: center;
  gap: 18px;
  padding: 18px;
  border: 1px solid var(--line);
  border-radius: 13px;
  background: #fff;
}
.route-card article {
  display: grid;
  gap: 7px;
}
.route-card article:last-child {
  text-align: right;
}
.route-card > span {
  color: var(--blue);
  font-size: 22px;
}
.route-card small,
.facts small,
.reason-card small,
.salary-card small {
  color: var(--muted);
  font-size: 9px;
  font-weight: 800;
  letter-spacing: 0.05em;
  text-transform: uppercase;
}
.route-card strong {
  font-size: 14px;
}
.facts {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-top: 12px;
}
.facts article {
  display: grid;
  gap: 6px;
  padding: 13px;
  border: 1px solid var(--line);
  border-radius: 10px;
  background: #fff;
}
.facts strong {
  font-size: 11px;
}
.reason-card {
  display: grid;
  gap: 8px;
  margin-top: 12px;
  padding: 14px;
  border-left: 3px solid #9fc4e9;
  border-radius: 0 10px 10px 0;
  background: #f1f6fb;
}
.reason-card p {
  color: #435a72;
  font-size: 11px;
  line-height: 1.7;
}
.salary-card {
  display: grid;
  gap: 7px;
  padding: 20px;
  border: 1px solid #cfe4dc;
  border-radius: 13px;
  background: #f0faf6;
}
.salary-card strong {
  color: var(--green);
  font: 800 28px ui-monospace, SFMono-Regular, Menlo, monospace;
}
.salary-card strong em {
  margin-left: 5px;
  font: 700 10px system-ui;
}
.salary-card span {
  color: #61798d;
  font-size: 10px;
}
.sod-note {
  margin-top: 12px;
  color: var(--muted);
  font-size: 9px;
  line-height: 1.7;
}
.decision-actions {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  align-items: end;
  gap: 14px;
  padding: 14px 20px;
  border-top: 1px solid var(--line);
}
.decision-actions label {
  display: grid;
  gap: 6px;
}
.decision-actions label span {
  color: var(--muted);
  font-size: 9px;
  font-weight: 800;
}
.decision-actions textarea {
  min-height: 54px;
  padding: 9px 10px;
  border: 1px solid var(--line);
  border-radius: 9px;
  resize: none;
  box-sizing: border-box;
  outline: none;
}
.decision-actions > div {
  display: flex;
  gap: 8px;
}
.decision-actions button {
  height: 39px;
  padding: 0 15px;
  border-radius: 9px;
  font-size: 10px;
  font-weight: 900;
  cursor: pointer;
}
.decision-actions button:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}
.reject {
  border: 1px solid #efcccc;
  background: #fff;
  color: var(--red);
}
.approve {
  border: 1px solid var(--green);
  background: var(--green);
  color: #fff;
}
.empty {
  display: grid;
  place-items: center;
  align-content: center;
  gap: 7px;
  min-height: 180px;
  color: var(--muted);
  text-align: center;
}
.empty > span {
  display: grid;
  place-items: center;
  width: 42px;
  height: 42px;
  border-radius: 13px;
  background: #edf8f4;
  color: var(--green);
  font-weight: 900;
}
.empty strong {
  color: #425a72;
  font-size: 12px;
}
.empty small {
  font-size: 9px;
}
.detail-empty {
  grid-row: 1 / -1;
  height: 100%;
}
.feedback {
  position: fixed;
  right: 24px;
  bottom: 22px;
  z-index: 20;
  padding: 11px 15px;
  border: 1px solid #cbe8dd;
  border-radius: 9px;
  background: #effaf6;
  color: var(--green);
  font-size: 10px;
  font-weight: 800;
  box-shadow: 0 12px 30px rgba(29, 58, 88, 0.13);
}
.feedback.error {
  border-color: #efcccc;
  background: #fff1f1;
  color: var(--red);
}
@media (max-width: 1000px) {
  .approval-shell {
    grid-template-columns: 290px minmax(0, 1fr);
  }
}
</style>
