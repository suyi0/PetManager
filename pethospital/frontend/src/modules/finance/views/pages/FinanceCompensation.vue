<template>
  <section class="activation-page">
    <header class="page-head">
      <div>
        <span class="eyebrow">SALARY PROFILE ACTIVATION</span>
        <h1>薪资配置激活</h1>
        <p>财务只核对并激活已批准提案；金额有误必须退回，不能在此直接改写。</p>
      </div>
      <div class="head-actions">
        <RouterLink to="/finance/salary">返回工资台账</RouterLink>
        <button type="button" :disabled="loading" @click="loadQueue">
          刷新队列
        </button>
      </div>
    </header>

    <div class="activation-shell">
      <aside class="queue-panel panel">
        <div class="panel-head">
          <div>
            <h2>待激活提案</h2>
            <span>{{ proposals.length }} 条</span>
          </div>
          <span class="scope">按组织快照</span>
        </div>
        <div class="queue-list">
          <button
            v-for="proposal in proposals"
            :key="proposal.id"
            type="button"
            class="queue-row"
            :class="{ selected: selected?.id === proposal.id }"
            @click="selectProposal(proposal)"
          >
            <span class="money-icon">¥</span>
            <span class="queue-copy">
              <strong>{{
                proposal.userName || `职工 #${proposal.userId}`
              }}</strong>
              <small
                >{{ phaseLabel(proposal.phase) }} ·
                {{
                  proposal.departmentName || `部门 #${proposal.departmentId}`
                }}</small
              >
            </span>
            <span class="date">{{ proposal.effectiveFrom }}</span>
          </button>
          <div v-if="!loading && proposals.length === 0" class="empty">
            <span>✓</span><strong>没有待激活提案</strong
            ><small>管理层批准后会进入此队列。</small>
          </div>
        </div>
      </aside>

      <main class="detail-panel panel">
        <template v-if="selected">
          <section class="detail-head">
            <div>
              <span class="eyebrow">APPROVED CASE #{{ selected.id }}</span>
              <h2>{{ selected.userName || `职工 #${selected.userId}` }}</h2>
              <p>
                {{
                  selected.positionName || `任职档案 #${selected.employmentId}`
                }}
                · row v{{ selected.rowVersion }}
              </p>
            </div>
            <span class="approved-mark">管理已批准</span>
          </section>

          <div class="detail-body">
            <section class="amount-card">
              <div>
                <small>{{
                  selected.payType === "hourly" ? "小时薪资" : "基础月薪"
                }}</small>
                <strong
                  >{{
                    money(
                      selected.payType === "hourly"
                        ? selected.hourlyRate
                        : selected.baseSalary
                    )
                  }}<em>{{
                    selected.payType === "hourly" ? "/小时" : "/月"
                  }}</em></strong
                >
              </div>
              <div>
                <small>五险一金</small
                ><b>{{ money(selected.socialInsuranceHousingFund) }}</b>
              </div>
              <div>
                <small>计划生效</small><b>{{ selected.effectiveFrom }}</b>
              </div>
            </section>

            <section class="comparison-grid">
              <article>
                <span class="card-label">当前生效配置</span>
                <template v-if="currentProfile">
                  <strong>{{
                    currentProfile.pay_type === "hourly"
                      ? `${money(currentProfile.hourly_rate)}/小时`
                      : `${money(currentProfile.base_salary)}/月`
                  }}</strong>
                  <small
                    >生效 {{ currentProfile.effective_from }} · 五险一金
                    {{
                      money(currentProfile.social_insurance_housing_fund)
                    }}</small
                  >
                </template>
                <template v-else
                  ><strong>尚无生效配置</strong
                  ><small>本次确认将创建第一条 salaryProfile。</small></template
                >
              </article>
              <article>
                <span class="card-label">任职状态迁移</span>
                <strong>{{ transitionLabel(selected) }}</strong>
                <small
                  >确认时同时校验任职版本 v{{
                    selected.expectedEmploymentRowVersion || "-"
                  }}，冲突会拒绝写入。</small
                >
              </article>
            </section>

            <section class="facts">
              <article>
                <small>组织快照</small
                ><strong
                  >{{ selected.branchName || `分院 #${selected.branchId}` }} ·
                  {{
                    selected.departmentName || `部门 #${selected.departmentId}`
                  }}</strong
                >
              </article>
              <article>
                <small>人事受理人</small
                ><strong>#{{ selected.assigneeUserId }}</strong>
              </article>
              <article>
                <small>管理审批人</small
                ><strong>#{{ selected.approvedBy || "-" }}</strong>
              </article>
              <article>
                <small>职责分离</small><strong>{{ separationHint }}</strong>
              </article>
            </section>

            <section class="proposal-note">
              <small>提案说明</small>
              <p>{{ selected.note || "未填写提案说明" }}</p>
            </section>
          </div>

          <footer class="actions">
            <label
              ><span>财务核对意见（退回时必填）</span
              ><textarea
                v-model.trim="reason"
                placeholder="记录预算、合同或薪酬凭证核对结果"
              />
            </label>
            <div>
              <button
                type="button"
                class="return"
                :disabled="saving || !reason"
                @click="returnProposal"
              >
                退回人事
              </button>
              <button
                type="button"
                class="confirm"
                :disabled="saving"
                @click="confirmProposal"
              >
                确认并激活
              </button>
            </div>
          </footer>
        </template>

        <div v-else class="empty detail-empty">
          <span>03</span><strong>选择一条已批准提案</strong
          ><small>确认前对比当前配置、任职版本与生效日期。</small>
        </div>
      </main>
    </div>

    <p v-if="feedback" class="feedback" :class="feedbackType">{{ feedback }}</p>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref } from "vue";
import { getHttpErrorMessage } from "@/api/httpError";
import { CompensationProposal } from "@/shared/types/employmentWorkflow";
import { financeApi } from "../../api/financeApi";
import { SalaryProfilePayload } from "../../api/types";

export default defineComponent({
  name: "FinanceCompensation",
  setup() {
    const proposals = ref<CompensationProposal[]>([]);
    const selected = ref<CompensationProposal | null>(null);
    const currentProfile = ref<SalaryProfilePayload | null>(null);
    const reason = ref("");
    const loading = ref(false);
    const saving = ref(false);
    const feedback = ref("");
    const feedbackType = ref<"success" | "error">("success");

    const loadQueue = async () => {
      loading.value = true;
      try {
        const result = await financeApi.listCompensationActivations();
        proposals.value = result.items;
        if (selected.value) {
          selected.value =
            result.items.find((item) => item.id === selected.value?.id) ?? null;
        }
      } catch (error) {
        feedback.value = getHttpErrorMessage(error, "薪资激活队列加载失败");
        feedbackType.value = "error";
      } finally {
        loading.value = false;
      }
    };
    const selectProposal = async (proposal: CompensationProposal) => {
      selected.value = proposal;
      reason.value = "";
      currentProfile.value = null;
      if (!proposal.userId) return;
      try {
        currentProfile.value = await financeApi.getSalaryProfile(
          proposal.userId
        );
      } catch {
        currentProfile.value = null;
      }
    };
    const finish = async (action: "confirm" | "return") => {
      if (!selected.value) return;
      saving.value = true;
      feedback.value = "";
      try {
        if (action === "confirm") {
          await financeApi.confirmCompensationActivation(
            selected.value.id,
            selected.value.rowVersion,
            reason.value || "财务核对通过"
          );
          feedback.value =
            "薪资配置已激活，任职状态与 salaryProfile 已原子更新";
        } else {
          await financeApi.returnCompensationActivation(
            selected.value.id,
            selected.value.rowVersion,
            reason.value
          );
          feedback.value = "提案已退回人事修改";
        }
        feedbackType.value = "success";
        selected.value = null;
        currentProfile.value = null;
        reason.value = "";
        await loadQueue();
      } catch (error) {
        feedback.value = getHttpErrorMessage(error, "财务处理失败");
        feedbackType.value = "error";
      } finally {
        saving.value = false;
      }
    };
    const confirmProposal = () => void finish("confirm");
    const returnProposal = () => void finish("return");
    const phaseLabel = (phase: string) =>
      ((
        {
          probation: "试用期薪酬",
          regular: "正式薪酬",
          adjustment: "在职调薪",
        } as Record<string, string>
      )[phase] || phase);
    const money = (value?: number | null) =>
      `￥${Number(value ?? 0).toLocaleString("zh-CN", {
        minimumFractionDigits: 2,
        maximumFractionDigits: 2,
      })}`;
    const transitionLabel = (proposal: CompensationProposal) => {
      if (proposal.phase === "probation") return "入职处理中 → 试用期";
      if (proposal.phase === "adjustment")
        return "正式在职 → 正式在职（版本递增）";
      return proposal.employmentStatus === "onboarding"
        ? "入职处理中 → 正式在职（免试用）"
        : "待正式薪酬 → 正式在职";
    };
    const separationHint = computed(() => {
      if (!selected.value) return "";
      return `拟案 #${selected.value.proposedBy} / 审批 #${
        selected.value.approvedBy || "-"
      } / 财务当前账号需不同`;
    });

    onMounted(() => void loadQueue());
    return {
      confirmProposal,
      currentProfile,
      feedback,
      feedbackType,
      loadQueue,
      loading,
      money,
      phaseLabel,
      proposals,
      reason,
      returnProposal,
      saving,
      selectProposal,
      selected,
      separationHint,
      transitionLabel,
    };
  },
});
</script>

<style scoped>
.activation-page {
  --ink: #14283e;
  --muted: #71859a;
  --line: #dbe6f1;
  --blue: #2f80cc;
  --green: #168364;
  --red: #ca5252;
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
  gap: 20px;
  min-height: 70px;
}
.eyebrow {
  color: #7b90a8;
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
.head-actions {
  display: flex;
  gap: 8px;
}
.head-actions a,
.head-actions button {
  display: inline-flex;
  align-items: center;
  height: 39px;
  padding: 0 14px;
  border: 1px solid var(--line);
  border-radius: 9px;
  background: #fff;
  color: #435b73;
  font-size: 10px;
  font-weight: 800;
  text-decoration: none;
  cursor: pointer;
}
.activation-shell {
  display: grid;
  grid-template-columns: minmax(300px, 0.7fr) minmax(620px, 1.45fr);
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
.panel-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 17px 18px;
  border-bottom: 1px solid var(--line);
}
.panel-head div {
  display: grid;
  gap: 3px;
}
.panel-head h2 {
  font-size: 15px;
}
.panel-head span {
  color: var(--muted);
  font-size: 10px;
}
.scope {
  padding: 5px 8px;
  border-radius: 999px;
  background: #f0f5fa;
  font-weight: 800;
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
  border-color: #cce6db;
  background: #eef9f5;
}
.money-icon {
  display: grid;
  place-items: center;
  width: 33px;
  height: 33px;
  border-radius: 10px;
  background: #e9f7f1;
  color: var(--green);
  font-weight: 900;
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
.date {
  color: var(--muted);
  font-size: 9px;
}
.detail-panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr) auto;
}
.detail-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 15px;
  padding: 19px 22px;
  border-bottom: 1px solid var(--line);
}
.detail-head h2 {
  margin-top: 4px;
  font-size: 20px;
}
.detail-head p {
  margin-top: 5px;
  color: var(--muted);
  font-size: 10px;
}
.approved-mark {
  padding: 6px 9px;
  border: 1px solid #c9e6db;
  border-radius: 7px;
  background: #edf9f4;
  color: var(--green);
  font-size: 10px;
  font-weight: 900;
}
.detail-body {
  min-height: 0;
  overflow: auto;
  padding: 20px 22px;
  background: #fbfcfe;
}
.amount-card {
  display: grid;
  grid-template-columns: 1.5fr 0.8fr 0.8fr;
  align-items: end;
  gap: 16px;
  padding: 18px;
  border: 1px solid #cde5dc;
  border-radius: 13px;
  background: #f0faf6;
}
.amount-card > div {
  display: grid;
  gap: 7px;
}
.amount-card small,
.facts small,
.proposal-note small,
.card-label {
  color: var(--muted);
  font-size: 9px;
  font-weight: 800;
  letter-spacing: 0.05em;
  text-transform: uppercase;
}
.amount-card strong {
  color: var(--green);
  font: 800 27px ui-monospace, SFMono-Regular, Menlo, monospace;
}
.amount-card strong em {
  margin-left: 5px;
  font: 700 10px system-ui;
}
.amount-card b {
  font-size: 13px;
}
.comparison-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 11px;
  margin-top: 12px;
}
.comparison-grid article {
  display: grid;
  gap: 7px;
  padding: 14px;
  border: 1px solid var(--line);
  border-radius: 11px;
  background: #fff;
}
.comparison-grid strong {
  font-size: 12px;
}
.comparison-grid small {
  color: var(--muted);
  font-size: 9px;
  line-height: 1.6;
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
  padding: 12px 13px;
  border: 1px solid var(--line);
  border-radius: 10px;
  background: #fff;
}
.facts strong {
  font-size: 10px;
  line-height: 1.5;
}
.proposal-note {
  display: grid;
  gap: 8px;
  margin-top: 12px;
  padding: 13px;
  border-left: 3px solid #9cc1e5;
  border-radius: 0 10px 10px 0;
  background: #f1f6fb;
}
.proposal-note p {
  color: #445c73;
  font-size: 10px;
  line-height: 1.7;
}
.actions {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  align-items: end;
  gap: 14px;
  padding: 14px 20px;
  border-top: 1px solid var(--line);
}
.actions label {
  display: grid;
  gap: 6px;
}
.actions label span {
  color: var(--muted);
  font-size: 9px;
  font-weight: 800;
}
.actions textarea {
  min-height: 54px;
  padding: 9px 10px;
  border: 1px solid var(--line);
  border-radius: 9px;
  resize: none;
  box-sizing: border-box;
  outline: none;
}
.actions > div {
  display: flex;
  gap: 8px;
}
.actions button {
  height: 39px;
  padding: 0 14px;
  border-radius: 9px;
  font-size: 10px;
  font-weight: 900;
  cursor: pointer;
}
.actions button:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}
.return {
  border: 1px solid #edcccc;
  background: #fff;
  color: var(--red);
}
.confirm {
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
  background: #eaf7f1;
  color: var(--green);
  font-weight: 900;
}
.empty strong {
  color: #435b72;
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
@media (max-width: 1050px) {
  .activation-shell {
    grid-template-columns: 290px minmax(0, 1fr);
  }
}
</style>
