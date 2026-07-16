<template>
  <section class="workspace">
    <header class="workspace__head">
      <div>
        <span class="eyebrow">COMPENSATION CASES</span>
        <h1>薪资管理</h1>
        <p>为已建档职工创建与处理薪酬提案；金额仅对当前受理人可见。</p>
      </div>
      <div class="head-actions">
        <label class="search-box">
          <span>⌕</span>
          <input
            v-model.trim="keywordInput"
            placeholder="搜索姓名 / 手机 / 邮箱"
            @keyup.enter="applySearch"
          />
        </label>
        <button class="button button--ghost" type="button" @click="refreshAll">
          刷新
        </button>
      </div>
    </header>

    <div class="workspace__body">
      <aside class="employee-panel panel">
        <div class="panel__head">
          <div>
            <h2>职工与候选人</h2>
            <span>{{ total }} 条可管理记录</span>
          </div>
          <span class="scope-mark">组织范围内</span>
        </div>

        <div class="employee-list" :class="{ 'is-loading': loadingEmployees }">
          <button
            v-for="employee in employees"
            :key="employee.id"
            type="button"
            class="employee-row"
            :class="{ selected: selectedEmployee?.id === employee.id }"
            @click="selectEmployee(employee)"
          >
            <span class="avatar">{{ initialOf(employee.name) }}</span>
            <span class="employee-copy">
              <strong>{{ employee.name || "未命名用户" }}</strong>
              <small>
                {{ employee.departmentName || "未归属部门" }} ·
                {{ employee.positionName || "待入职派岗" }}
              </small>
            </span>
            <span
              class="state-chip"
              :class="statusTone(employee.employmentStatus)"
            >
              {{ employmentStatusLabel(employee) }}
            </span>
          </button>

          <div
            v-if="!loadingEmployees && employees.length === 0"
            class="empty-state"
          >
            <strong>没有匹配记录</strong>
            <span>可尝试姓名、手机号或邮箱关键词。</span>
          </div>
        </div>

        <footer class="panel__foot">
          <button type="button" :disabled="page <= 1" @click="page -= 1">
            上一页
          </button>
          <span>{{ page }} / {{ totalPages }}</span>
          <button
            type="button"
            :disabled="page >= totalPages"
            @click="page += 1"
          >
            下一页
          </button>
        </footer>
      </aside>

      <main class="detail-panel panel">
        <template v-if="selectedEmployee">
          <section class="employee-summary">
            <div class="summary-person">
              <span class="avatar avatar--large">{{
                initialOf(selectedEmployee.name)
              }}</span>
              <div>
                <div class="summary-title">
                  <h2>{{ selectedEmployee.name || "未命名用户" }}</h2>
                  <span
                    class="state-chip"
                    :class="statusTone(selectedEmployee.employmentStatus)"
                  >
                    {{ employmentStatusLabel(selectedEmployee) }}
                  </span>
                </div>
                <p>
                  #{{ selectedEmployee.id }} ·
                  {{
                    selectedEmployee.phone ||
                    selectedEmployee.email ||
                    "暂无联系方式"
                  }}
                </p>
              </div>
            </div>
            <div class="summary-facts">
              <span
                ><small>部门</small
                >{{ selectedEmployee.departmentName || "待分配" }}</span
              >
              <span
                ><small>职位</small
                >{{ selectedEmployee.positionName || "待分配" }}</span
              >
              <span
                ><small>任职档案</small
                >{{
                  selectedEmployee.employmentId
                    ? `#${selectedEmployee.employmentId}`
                    : "尚未建立"
                }}</span
              >
              <span
                ><small>薪酬案件</small>{{ employeeProposals.length }} 条</span
              >
            </div>
          </section>

          <div class="tab-content compensation-grid">
            <section class="proposal-list-card">
              <div class="section-title">
                <div>
                  <span class="eyebrow">CASE HISTORY</span>
                  <h3>该职工的薪酬案件</h3>
                </div>
                <button
                  class="text-button"
                  type="button"
                  @click="startNewProposal"
                >
                  新建提案
                </button>
              </div>
              <div class="proposal-list">
                <button
                  v-for="proposal in employeeProposals"
                  :key="proposal.id"
                  type="button"
                  class="proposal-row"
                  :class="{ selected: selectedProposal?.id === proposal.id }"
                  @click="selectProposal(proposal)"
                >
                  <span>
                    <strong>{{ phaseLabel(proposal.phase) }}</strong>
                    <small
                      >#{{ proposal.id }} · 生效
                      {{ proposal.effectiveFrom }}</small
                    >
                  </span>
                  <span
                    class="state-chip"
                    :class="proposalStatusTone(proposal.status)"
                  >
                    {{ proposalStatusLabel(proposal.status) }}
                  </span>
                </button>
                <div
                  v-if="employeeProposals.length === 0"
                  class="empty-state compact"
                >
                  <strong>暂无薪酬案件</strong>
                  <span
                    >先完成任职档案，再由人事建立试用、正式或调薪提案。</span
                  >
                </div>
              </div>
            </section>

            <section class="proposal-editor form-card">
              <div class="section-title">
                <div>
                  <span class="eyebrow">COMPENSATION PROPOSAL</span>
                  <h3>
                    {{
                      selectedProposal
                        ? `提案 #${selectedProposal.id}`
                        : "新建薪酬提案"
                    }}
                  </h3>
                </div>
                <span
                  v-if="selectedProposal"
                  class="state-chip"
                  :class="proposalStatusTone(selectedProposal.status)"
                >
                  {{ proposalStatusLabel(selectedProposal.status) }}
                </span>
              </div>

              <div
                v-if="
                  selectedProposal && selectedProposal.payType === undefined
                "
                class="masked-note"
              >
                你可查看案件状态，但金额仅对当前受理人、审批人和财务激活人开放。
              </div>

              <div class="form-grid">
                <label>
                  <span>薪酬阶段</span>
                  <select
                    v-model="proposalForm.phase"
                    :disabled="Boolean(selectedProposal)"
                  >
                    <option value="probation">试用期薪酬</option>
                    <option value="regular">正式薪酬</option>
                    <option value="adjustment">在职调薪</option>
                  </select>
                </label>
                <label>
                  <span>计薪方式</span>
                  <select
                    v-model="proposalForm.payType"
                    :disabled="!canEditProposal"
                  >
                    <option value="monthly">月薪</option>
                    <option value="hourly">时薪</option>
                  </select>
                </label>
                <label>
                  <span>{{
                    proposalForm.payType === "monthly" ? "基础月薪" : "小时薪资"
                  }}</span>
                  <input
                    v-if="proposalForm.payType === 'monthly'"
                    v-model.number="proposalForm.baseSalary"
                    type="number"
                    min="0"
                    :disabled="!canEditProposal"
                  />
                  <input
                    v-else
                    v-model.number="proposalForm.hourlyRate"
                    type="number"
                    min="0"
                    :disabled="!canEditProposal"
                  />
                </label>
                <label>
                  <span>五险一金</span>
                  <input
                    v-model.number="proposalForm.social"
                    type="number"
                    min="0"
                    :disabled="!canEditProposal"
                  />
                </label>
                <label>
                  <span>生效日期</span>
                  <input
                    v-model="proposalForm.effectiveFrom"
                    type="date"
                    :disabled="!canEditProposal"
                  />
                </label>
                <label class="span-2">
                  <span>提案说明</span>
                  <textarea
                    v-model.trim="proposalForm.note"
                    :disabled="!canEditProposal"
                    placeholder="说明薪资承诺、试用期约定或调薪依据"
                  />
                </label>
              </div>

              <div class="proposal-actions">
                <button
                  class="button button--primary"
                  type="button"
                  :disabled="
                    saving || !canEditProposal || !selectedEmployee.employmentId
                  "
                  @click="saveProposal"
                >
                  {{ selectedProposal ? "保存草稿" : "创建草稿" }}
                </button>
                <button
                  class="button button--success"
                  type="button"
                  :disabled="saving || !selectedProposal || !canEditProposal"
                  @click="submitProposal"
                >
                  提交管理审批
                </button>
              </div>

              <div v-if="selectedProposal && canReassign" class="reassign-box">
                <label
                  ><span>改派给人事用户 ID</span
                  ><input v-model.number="reassignUserId" type="number" min="1"
                /></label>
                <label
                  ><span>改派原因</span
                  ><input v-model.trim="reassignReason" placeholder="必填"
                /></label>
                <button
                  type="button"
                  :disabled="saving || !reassignUserId || !reassignReason"
                  @click="reassignProposal"
                >
                  改派案件
                </button>
              </div>

              <p
                v-if="!selectedEmployee.employmentId"
                class="guard-note is-error"
              >
                当前响应缺少任职档案
                ID，不能创建薪酬提案；请先在「职位任职」页完成入职派岗。
              </p>
            </section>
          </div>
        </template>

        <div v-else class="empty-detail">
          <span>02</span>
          <h2>选择一名职工</h2>
          <p>左侧选择后，可查看并处理该职工的薪酬案件与提案。</p>
        </div>
      </main>
    </div>

    <div v-if="message.text" class="toast" :class="message.type">
      <strong>{{
        message.type === "error" ? "操作未完成" : "操作完成"
      }}</strong>
      <span>{{ message.text }}</span>
      <button type="button" @click="message.text = ''">×</button>
    </div>
  </section>
</template>

<script lang="ts">
import {
  computed,
  defineComponent,
  onMounted,
  reactive,
  ref,
  watch,
} from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { getHttpErrorMessage } from "@/api/httpError";
import {
  CompensationPhase,
  CompensationProposal,
  PersonnelEmployee,
} from "@/shared/types/employmentWorkflow";
import { personnelApi } from "../../api/personnelApi";
import {
  employmentStatusLabel,
  initialOf,
  phaseLabel,
  proposalStatusLabel,
  proposalStatusTone,
  statusTone,
  today,
} from "../../utils/personnelLabels";

export default defineComponent({
  name: "PersonnelCompensation",
  setup() {
    const store = useStore(storeKey);
    const employees = ref<PersonnelEmployee[]>([]);
    const selectedEmployee = ref<PersonnelEmployee | null>(null);
    const proposals = ref<CompensationProposal[]>([]);
    const selectedProposal = ref<CompensationProposal | null>(null);
    const keywordInput = ref("");
    const keyword = ref("");
    const page = ref(1);
    const pageSize = 12;
    const total = ref(0);
    const loadingEmployees = ref(false);
    const saving = ref(false);
    const reassignUserId = ref<number | null>(null);
    const reassignReason = ref("");
    const message = reactive<{ text: string; type: "success" | "error" }>({
      text: "",
      type: "success",
    });

    const proposalForm = reactive({
      phase: "probation" as CompensationPhase,
      payType: "monthly" as "monthly" | "hourly",
      baseSalary: 0,
      hourlyRate: 0,
      social: 0,
      effectiveFrom: today(),
      note: "",
    });

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(total.value / pageSize))
    );
    const employeeProposals = computed(() => {
      if (!selectedEmployee.value) return [];
      return proposals.value.filter(
        (proposal) =>
          (selectedEmployee.value?.employmentId &&
            proposal.employmentId === selectedEmployee.value.employmentId) ||
          proposal.userId === selectedEmployee.value?.id
      );
    });
    const canReassign = computed(() =>
      store.state.auth.permissions.includes("compensation:reassign-case")
    );
    const canEditProposal = computed(
      () =>
        !selectedProposal.value ||
        (["draft", "returned"].includes(selectedProposal.value.status) &&
          selectedProposal.value.payType !== undefined)
    );

    const notify = (text: string, type: "success" | "error" = "success") => {
      message.text = text;
      message.type = type;
    };

    const defaultPhase = (employee: PersonnelEmployee): CompensationPhase => {
      if (employee.employmentStatus === "active") return "adjustment";
      if (
        employee.employmentStatus === "probation" ||
        employee.employmentStatus === "regularization_pending"
      )
        return "regular";
      return employee.probationWaived ? "regular" : "probation";
    };

    const loadEmployees = async () => {
      loadingEmployees.value = true;
      try {
        const result = await personnelApi.searchEmployees({
          keyword: keyword.value,
          page: page.value,
          pageSize,
        });
        employees.value = result.items;
        total.value = result.total;
        if (selectedEmployee.value) {
          const latest = result.items.find(
            (item) => item.id === selectedEmployee.value?.id
          );
          if (latest)
            selectedEmployee.value = { ...selectedEmployee.value, ...latest };
        } else if (result.items[0]) {
          await selectEmployee(result.items[0]);
        }
      } catch (error) {
        notify(getHttpErrorMessage(error, "职工列表加载失败"), "error");
      } finally {
        loadingEmployees.value = false;
      }
    };

    const loadProposals = async () => {
      try {
        const result = await personnelApi.listCompensationProposals({
          page: 1,
          pageSize: 100,
        });
        proposals.value = result.items;
      } catch (error) {
        proposals.value = [];
        if (store.state.auth.permissions.includes("compensation:propose")) {
          notify(getHttpErrorMessage(error, "薪酬案件加载失败"), "error");
        }
      }
    };

    const selectEmployee = async (employee: PersonnelEmployee) => {
      selectedEmployee.value = employee;
      selectedProposal.value = null;
      try {
        const detail = await personnelApi.getEmployee(employee.id);
        selectedEmployee.value = { ...employee, ...detail };
        proposalForm.phase = defaultPhase(detail);
      } catch (error) {
        notify(getHttpErrorMessage(error, "职工详情加载失败"), "error");
      }
    };

    const refreshAll = async () => {
      await Promise.all([loadEmployees(), loadProposals()]);
    };

    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
      page.value = 1;
      selectedEmployee.value = null;
      void loadEmployees();
    };

    const resetProposalForm = () => {
      selectedProposal.value = null;
      proposalForm.phase = selectedEmployee.value
        ? defaultPhase(selectedEmployee.value)
        : "probation";
      proposalForm.payType = "monthly";
      proposalForm.baseSalary = 0;
      proposalForm.hourlyRate = 0;
      proposalForm.social = 0;
      proposalForm.effectiveFrom = today();
      proposalForm.note = "";
      reassignUserId.value = null;
      reassignReason.value = "";
    };
    const startNewProposal = () => resetProposalForm();
    const selectProposal = (proposal: CompensationProposal) => {
      selectedProposal.value = proposal;
      if (proposal.payType) proposalForm.payType = proposal.payType;
      proposalForm.phase = proposal.phase;
      proposalForm.baseSalary = Number(proposal.baseSalary ?? 0);
      proposalForm.hourlyRate = Number(proposal.hourlyRate ?? 0);
      proposalForm.social = Number(proposal.socialInsuranceHousingFund ?? 0);
      proposalForm.effectiveFrom = proposal.effectiveFrom;
      proposalForm.note = proposal.note ?? "";
      reassignUserId.value = proposal.assigneeUserId || null;
      reassignReason.value = "";
    };

    const saveProposal = async () => {
      if (!selectedEmployee.value?.employmentId || !canEditProposal.value)
        return;
      saving.value = true;
      const payload = {
        payType: proposalForm.payType,
        baseSalary:
          proposalForm.payType === "monthly"
            ? Number(proposalForm.baseSalary)
            : null,
        hourlyRate:
          proposalForm.payType === "hourly"
            ? Number(proposalForm.hourlyRate)
            : null,
        socialInsuranceHousingFund: Number(proposalForm.social),
        effectiveFrom: proposalForm.effectiveFrom,
        note: proposalForm.note,
      };
      try {
        const proposal = selectedProposal.value
          ? await personnelApi.updateCompensationProposal(
              selectedProposal.value.id,
              {
                ...payload,
                expectedRowVersion: selectedProposal.value.rowVersion,
              }
            )
          : await personnelApi.createCompensationProposal({
              employmentId: selectedEmployee.value.employmentId,
              phase: proposalForm.phase,
              ...payload,
            });
        notify(selectedProposal.value ? "薪酬草稿已更新" : "薪酬草稿已创建");
        await loadProposals();
        selectProposal(
          proposals.value.find((item) => item.id === proposal.id) ?? proposal
        );
      } catch (error) {
        notify(getHttpErrorMessage(error, "薪酬草稿保存失败"), "error");
      } finally {
        saving.value = false;
      }
    };

    const submitProposal = async () => {
      if (!selectedProposal.value || !canEditProposal.value) return;
      saving.value = true;
      try {
        await personnelApi.submitCompensationProposal(
          selectedProposal.value.id,
          selectedProposal.value.rowVersion,
          proposalForm.note
        );
        notify("薪酬提案已提交管理审批");
        await loadProposals();
        const latest = proposals.value.find(
          (item) => item.id === selectedProposal.value?.id
        );
        if (latest) selectProposal(latest);
      } catch (error) {
        notify(getHttpErrorMessage(error, "薪酬提案提交失败"), "error");
      } finally {
        saving.value = false;
      }
    };

    const reassignProposal = async () => {
      if (
        !selectedProposal.value ||
        !reassignUserId.value ||
        !reassignReason.value
      )
        return;
      saving.value = true;
      try {
        await personnelApi.reassignCompensationProposal(
          selectedProposal.value.id,
          reassignUserId.value,
          selectedProposal.value.rowVersion,
          reassignReason.value
        );
        notify("薪酬案件已改派");
        await loadProposals();
        const latest = proposals.value.find(
          (item) => item.id === selectedProposal.value?.id
        );
        if (latest) selectProposal(latest);
      } catch (error) {
        notify(getHttpErrorMessage(error, "案件改派失败"), "error");
      } finally {
        saving.value = false;
      }
    };

    watch(page, () => void loadEmployees());

    onMounted(async () => {
      await loadProposals();
      await loadEmployees();
    });

    return {
      applySearch,
      canEditProposal,
      canReassign,
      employeeProposals,
      employees,
      employmentStatusLabel,
      initialOf,
      keywordInput,
      loadingEmployees,
      message,
      page,
      phaseLabel,
      proposalForm,
      proposalStatusLabel,
      proposalStatusTone,
      reassignProposal,
      reassignReason,
      reassignUserId,
      refreshAll,
      saveProposal,
      saving,
      selectEmployee,
      selectProposal,
      selectedEmployee,
      selectedProposal,
      startNewProposal,
      statusTone,
      submitProposal,
      total,
      totalPages,
    };
  },
});
</script>

<style src="./personnelWorkspace.css"></style>
