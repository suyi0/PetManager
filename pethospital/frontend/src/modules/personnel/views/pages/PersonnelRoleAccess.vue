<template>
  <section class="workspace">
    <header class="workspace__head">
      <div>
        <span class="eyebrow">PERSONNEL OPERATIONS</span>
        <h1>任职与薪酬交接</h1>
        <p>按部门职位派岗；人事只处理任职流程和被指派的薪酬案件。</p>
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
            </div>
          </section>

          <nav class="tabs" aria-label="人事工作区">
            <button
              type="button"
              :class="{ active: activeTab === 'employment' }"
              @click="activeTab = 'employment'"
            >
              任职管理
            </button>
            <button
              type="button"
              :class="{ active: activeTab === 'compensation' }"
              @click="activeTab = 'compensation'"
            >
              薪酬案件
              <span>{{ employeeProposals.length }}</span>
            </button>
          </nav>

          <div
            v-if="activeTab === 'employment'"
            class="tab-content employment-grid"
          >
            <section class="form-card">
              <div class="section-title">
                <div>
                  <span class="eyebrow">POSITION ASSIGNMENT</span>
                  <h3>
                    {{
                      assignmentAction === "onboard"
                        ? "建立入职任职"
                        : "调整部门职位"
                    }}
                  </h3>
                </div>
                <span
                  v-if="selectedPosition"
                  class="policy-chip"
                  :class="selectedPosition.assignmentPolicy"
                >
                  {{ assignmentPolicyLabel(selectedPosition.assignmentPolicy) }}
                </span>
              </div>

              <div class="form-grid">
                <label>
                  <span>目标部门</span>
                  <select v-model.number="assignmentForm.departmentId">
                    <option :value="0">选择部门</option>
                    <option
                      v-for="department in departments"
                      :key="department.id"
                      :value="department.id"
                    >
                      {{
                        department.branchName
                          ? `${department.branchName} · `
                          : ""
                      }}{{ department.name }}
                    </option>
                  </select>
                </label>
                <label>
                  <span>目标职位</span>
                  <select v-model.number="assignmentForm.positionId">
                    <option :value="0">选择职位</option>
                    <option
                      v-for="position in filteredPositions"
                      :key="position.id"
                      :value="position.id"
                    >
                      {{ position.name }}
                    </option>
                  </select>
                </label>
                <label>
                  <span>生效日期</span>
                  <input v-model="assignmentForm.effectiveFrom" type="date" />
                </label>
                <label class="span-2">
                  <span>业务原因</span>
                  <textarea
                    v-model.trim="assignmentForm.reason"
                    placeholder="必填：说明入职、调岗或组织决定依据"
                  />
                </label>
              </div>

              <div v-if="selectedPosition" class="position-preview">
                <div>
                  <strong>{{ selectedPosition.name }}</strong>
                  <span>{{
                    selectedPosition.description || "该职位未填写说明"
                  }}</span>
                </div>
                <small
                  >权限来自职位模板，共
                  {{ selectedPosition.permissionSummary.length }}
                  项；人事不能逐项改权限。</small
                >
              </div>

              <button
                class="button button--primary button--wide"
                type="button"
                :disabled="
                  saving || !assignmentForm.positionId || !assignmentForm.reason
                "
                @click="saveAssignment"
              >
                {{
                  assignmentAction === "onboard"
                    ? "提交入职派岗"
                    : "提交职位调整"
                }}
              </button>
            </section>

            <section class="lifecycle-card">
              <div class="section-title">
                <div>
                  <span class="eyebrow">EMPLOYMENT LIFECYCLE</span>
                  <h3>任职生命周期</h3>
                </div>
              </div>

              <ol class="timeline">
                <li
                  v-for="step in lifecycleSteps"
                  :key="step.key"
                  :class="step.state"
                >
                  <span></span>
                  <div>
                    <strong>{{ step.label }}</strong
                    ><small>{{ step.hint }}</small>
                  </div>
                </li>
              </ol>

              <label class="lifecycle-reason">
                <span>转正 / 离职原因</span>
                <textarea
                  v-model.trim="lifecycleReason"
                  placeholder="发起生命周期变更时必填"
                />
              </label>
              <div class="lifecycle-actions">
                <button
                  class="button button--success"
                  type="button"
                  :disabled="saving || !canRegularize || !lifecycleReason"
                  @click="regularizeEmployee"
                >
                  发起转正
                </button>
                <button
                  class="button button--danger"
                  type="button"
                  :disabled="saving || !canOffboard || !lifecycleReason"
                  @click="offboardEmployee"
                >
                  发起离职
                </button>
              </div>
              <p class="guard-note">
                转正与离职均进入主管审批；页面不会用旧“移除角色”接口解除任职。
              </p>
            </section>
          </div>

          <div v-else class="tab-content compensation-grid">
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
                ID，不能创建薪酬提案；请刷新或先完成入职派岗。
              </p>
            </section>
          </div>
        </template>

        <div v-else class="empty-detail">
          <span>01</span>
          <h2>选择一名职工或候选人</h2>
          <p>
            左侧选择后，可按部门职位派岗、发起转正/离职，或处理指定薪酬案件。
          </p>
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
  DepartmentOption,
  PersonnelEmployee,
  PositionOption,
} from "@/shared/types/employmentWorkflow";
import { personnelApi } from "../../api/personnelApi";

const today = () => new Date().toISOString().slice(0, 10);

export default defineComponent({
  name: "PersonnelRoleAccess",
  setup() {
    const store = useStore(storeKey);
    const activeTab = ref<"employment" | "compensation">("employment");
    const employees = ref<PersonnelEmployee[]>([]);
    const selectedEmployee = ref<PersonnelEmployee | null>(null);
    const departments = ref<DepartmentOption[]>([]);
    const positions = ref<PositionOption[]>([]);
    const proposals = ref<CompensationProposal[]>([]);
    const selectedProposal = ref<CompensationProposal | null>(null);
    const keywordInput = ref("");
    const keyword = ref("");
    const page = ref(1);
    const pageSize = 12;
    const total = ref(0);
    const loadingEmployees = ref(false);
    const saving = ref(false);
    const lifecycleReason = ref("");
    const reassignUserId = ref<number | null>(null);
    const reassignReason = ref("");
    const message = reactive<{ text: string; type: "success" | "error" }>({
      text: "",
      type: "success",
    });

    const assignmentForm = reactive({
      departmentId: 0,
      positionId: 0,
      effectiveFrom: today(),
      reason: "",
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
    const filteredPositions = computed(() =>
      positions.value.filter(
        (position) =>
          !assignmentForm.departmentId ||
          position.departmentId === assignmentForm.departmentId
      )
    );
    const selectedPosition = computed(() =>
      positions.value.find(
        (position) => position.id === assignmentForm.positionId
      )
    );
    const assignmentAction = computed<"onboard" | "transfer">(() =>
      selectedEmployee.value?.accountType === "customer" ||
      !selectedEmployee.value?.positionId
        ? "onboard"
        : "transfer"
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
    const canRegularize = computed(
      () => selectedEmployee.value?.employmentStatus === "probation"
    );
    const canOffboard = computed(() =>
      ["probation", "regularization_pending", "active"].includes(
        selectedEmployee.value?.employmentStatus || ""
      )
    );
    const canReassign = computed(() =>
      store.state.auth.permissions.includes("compensation:reassign-case")
    );
    const canEditProposal = computed(
      () =>
        !selectedProposal.value ||
        (["draft", "returned"].includes(selectedProposal.value.status) &&
          selectedProposal.value.payType !== undefined)
    );

    const lifecycleSteps = computed(() => {
      const status = selectedEmployee.value?.employmentStatus || "draft";
      const order = [
        "onboarding",
        "probation",
        "regularization_pending",
        "active",
      ];
      const index = order.indexOf(status);
      return [
        {
          key: "onboarding",
          label: "入职任职",
          hint: "部门职位决定生效",
          state: index >= 0 ? "done" : "current",
        },
        {
          key: "probation",
          label: "试用阶段",
          hint: "试用薪酬由提案激活",
          state: index > 1 ? "done" : index === 1 ? "current" : "idle",
        },
        {
          key: "regularize",
          label: "转正决定",
          hint: "主管批准任职决定",
          state: index > 2 ? "done" : index === 2 ? "current" : "idle",
        },
        {
          key: "active",
          label: "正式在职",
          hint: "正式薪酬由财务激活",
          state: status === "active" ? "current" : "idle",
        },
      ];
    });

    const notify = (text: string, type: "success" | "error" = "success") => {
      message.text = text;
      message.type = type;
    };
    const initialOf = (name: string) => name.trim().charAt(0) || "?";
    const employmentStatusLabel = (employee: PersonnelEmployee) => {
      if (!employee.employmentStatus)
        return employee.accountType === "customer" ? "待入职" : "档案待同步";
      return (
        (
          {
            draft: "草稿",
            onboarding: "入职处理中",
            probation: "试用期",
            regularization_pending: "待正式薪酬",
            active: "正式在职",
            rejected: "已拒绝",
            separated: "已离职",
          } as Record<string, string>
        )[employee.employmentStatus] || employee.employmentStatus
      );
    };
    const statusTone = (status: string) =>
      status === "active"
        ? "success"
        : status === "separated" || status === "rejected"
        ? "danger"
        : status === "probation"
        ? "warning"
        : "neutral";
    const assignmentPolicyLabel = (policy: string) =>
      policy === "direct"
        ? "可直接生效"
        : policy === "approval_required"
        ? "需要主管审批"
        : "不可由人事派岗";
    const phaseLabel = (phase: string) =>
      ((
        {
          probation: "试用期薪酬",
          regular: "正式薪酬",
          adjustment: "在职调薪",
        } as Record<string, string>
      )[phase] || phase);
    const proposalStatusLabel = (status: string) =>
      ((
        {
          draft: "草稿",
          submitted: "待管理审批",
          management_approved: "待财务激活",
          returned: "已退回",
          finance_confirmed: "财务已确认",
          active: "已生效",
          cancelled: "已取消",
        } as Record<string, string>
      )[status] || status);
    const proposalStatusTone = (status: string) =>
      status === "active"
        ? "success"
        : status === "returned" || status === "cancelled"
        ? "danger"
        : status === "draft"
        ? "neutral"
        : "warning";

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
    const loadOrg = async () => {
      const [departmentItems, positionItems] = await Promise.all([
        personnelApi.getDepartments(),
        personnelApi.getPositions(),
      ]);
      departments.value = departmentItems;
      positions.value = positionItems;
    };
    const selectEmployee = async (employee: PersonnelEmployee) => {
      selectedEmployee.value = employee;
      selectedProposal.value = null;
      activeTab.value = "employment";
      assignmentForm.departmentId = employee.departmentId ?? 0;
      assignmentForm.positionId = employee.positionId ?? 0;
      assignmentForm.reason = "";
      lifecycleReason.value = "";
      try {
        const detail = await personnelApi.getEmployee(employee.id);
        selectedEmployee.value = { ...employee, ...detail };
        assignmentForm.departmentId = detail.departmentId ?? 0;
        assignmentForm.positionId = detail.positionId ?? 0;
        proposalForm.phase = defaultPhase(detail);
      } catch (error) {
        notify(getHttpErrorMessage(error, "职工详情加载失败"), "error");
      }
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
    const refreshSelected = async () => {
      if (!selectedEmployee.value) return;
      const detail = await personnelApi.getEmployee(selectedEmployee.value.id);
      selectedEmployee.value = { ...selectedEmployee.value, ...detail };
      await Promise.all([loadEmployees(), loadProposals()]);
    };
    const refreshAll = async () => {
      await Promise.all([loadEmployees(), loadProposals(), loadOrg()]);
    };
    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
      page.value = 1;
      selectedEmployee.value = null;
      void loadEmployees();
    };

    const saveAssignment = async () => {
      if (
        !selectedEmployee.value ||
        !assignmentForm.positionId ||
        !assignmentForm.reason
      )
        return;
      saving.value = true;
      try {
        const result = await personnelApi.updateAssignment(
          selectedEmployee.value.id,
          {
            action: assignmentAction.value,
            positionId: assignmentForm.positionId,
            expectedCurrentPositionId: selectedEmployee.value.positionId ?? 0,
            effectiveFrom: assignmentForm.effectiveFrom,
            reason: assignmentForm.reason,
          }
        );
        notify(
          result.assignmentStatus === "pending"
            ? "任职申请已提交主管审批"
            : "任职调整已生效"
        );
        await refreshSelected();
      } catch (error) {
        notify(getHttpErrorMessage(error, "任职调整失败"), "error");
      } finally {
        saving.value = false;
      }
    };
    const regularizeEmployee = async () => {
      if (!selectedEmployee.value || !lifecycleReason.value) return;
      saving.value = true;
      try {
        await personnelApi.regularize(selectedEmployee.value.id, {
          expectedCurrentPositionId: selectedEmployee.value.positionId ?? 0,
          effectiveFrom: assignmentForm.effectiveFrom,
          reason: lifecycleReason.value,
        });
        notify("转正任职申请已提交主管审批");
        await refreshSelected();
      } catch (error) {
        notify(getHttpErrorMessage(error, "转正申请失败"), "error");
      } finally {
        saving.value = false;
      }
    };
    const offboardEmployee = async () => {
      if (!selectedEmployee.value || !lifecycleReason.value) return;
      saving.value = true;
      try {
        await personnelApi.offboard(selectedEmployee.value.id, {
          expectedCurrentPositionId: selectedEmployee.value.positionId ?? 0,
          effectiveFrom: assignmentForm.effectiveFrom,
          reason: lifecycleReason.value,
        });
        notify("离职申请已提交主管审批");
        await refreshSelected();
      } catch (error) {
        notify(getHttpErrorMessage(error, "离职申请失败"), "error");
      } finally {
        saving.value = false;
      }
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
    watch(
      () => assignmentForm.departmentId,
      () => {
        if (
          assignmentForm.positionId &&
          !filteredPositions.value.some(
            (item) => item.id === assignmentForm.positionId
          )
        ) {
          assignmentForm.positionId = 0;
        }
      }
    );

    onMounted(async () => {
      const results = await Promise.allSettled([loadOrg(), loadProposals()]);
      const rejected = results.find(
        (result): result is PromiseRejectedResult =>
          result.status === "rejected"
      );
      if (rejected) {
        notify(
          getHttpErrorMessage(rejected.reason, "组织数据加载失败"),
          "error"
        );
      }
      await loadEmployees();
    });

    return {
      activeTab,
      applySearch,
      assignmentAction,
      assignmentForm,
      assignmentPolicyLabel,
      canEditProposal,
      canOffboard,
      canReassign,
      canRegularize,
      departments,
      employeeProposals,
      employees,
      employmentStatusLabel,
      filteredPositions,
      initialOf,
      keywordInput,
      lifecycleReason,
      lifecycleSteps,
      loadingEmployees,
      message,
      offboardEmployee,
      page,
      phaseLabel,
      proposalForm,
      proposalStatusLabel,
      proposalStatusTone,
      reassignProposal,
      reassignReason,
      reassignUserId,
      refreshAll,
      regularizeEmployee,
      saveAssignment,
      saveProposal,
      saving,
      selectEmployee,
      selectProposal,
      selectedEmployee,
      selectedPosition,
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

<style scoped>
.workspace {
  --ink: #132238;
  --muted: #70839a;
  --line: #dce6f1;
  --blue: #2f7fca;
  --blue-soft: #edf5ff;
  --green: #148566;
  --amber: #b87418;
  --red: #cb5151;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 14px;
  height: 100%;
  min-height: 0;
  color: var(--ink);
}

button,
input,
select,
textarea {
  font: inherit;
}

.workspace__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 20px;
  min-height: 70px;
}

.eyebrow {
  color: #7890aa;
  font-size: 10px;
  font-weight: 800;
  letter-spacing: 0.16em;
}

h1,
h2,
h3,
p {
  margin: 0;
}
.workspace__head h1 {
  margin-top: 4px;
  font-size: 25px;
  letter-spacing: -0.03em;
}
.workspace__head p {
  margin-top: 5px;
  color: var(--muted);
  font-size: 12px;
}
.head-actions {
  display: flex;
  align-items: center;
  gap: 9px;
}

.search-box {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 280px;
  height: 40px;
  padding: 0 13px;
  border: 1px solid var(--line);
  border-radius: 10px;
  background: #fff;
  box-sizing: border-box;
}
.search-box span {
  color: #94a6b8;
  font-size: 18px;
}
.search-box input {
  width: 100%;
  border: 0;
  outline: 0;
  color: var(--ink);
}

.workspace__body {
  display: grid;
  grid-template-columns: minmax(300px, 0.72fr) minmax(640px, 1.55fr);
  gap: 14px;
  min-height: 0;
}

.panel {
  min-height: 0;
  border: 1px solid var(--line);
  border-radius: 16px;
  background: #fff;
  box-shadow: 0 2px 8px rgba(31, 60, 91, 0.035);
  overflow: hidden;
}

.employee-panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr) auto;
}
.panel__head,
.panel__foot {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}
.panel__head {
  padding: 17px 18px;
  border-bottom: 1px solid var(--line);
}
.panel__head h2 {
  font-size: 15px;
}
.panel__head span {
  color: var(--muted);
  font-size: 11px;
}
.scope-mark {
  padding: 5px 8px;
  border-radius: 999px;
  background: #f0f5fa;
  font-weight: 700;
}
.employee-list {
  min-height: 0;
  overflow: auto;
  padding: 8px;
}
.employee-list.is-loading {
  opacity: 0.6;
  pointer-events: none;
}

.employee-row {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  align-items: center;
  gap: 11px;
  width: 100%;
  min-height: 66px;
  padding: 9px 10px;
  border: 1px solid transparent;
  border-radius: 11px;
  background: transparent;
  color: var(--ink);
  text-align: left;
  cursor: pointer;
}
.employee-row:hover {
  background: #f7faff;
}
.employee-row.selected {
  border-color: #cfe3f8;
  background: var(--blue-soft);
}
.avatar {
  display: grid;
  place-items: center;
  width: 34px;
  height: 34px;
  border-radius: 10px;
  background: #e7f0fb;
  color: #2b6daa;
  font-weight: 800;
}
.avatar--large {
  width: 45px;
  height: 45px;
  border-radius: 13px;
  font-size: 17px;
}
.employee-copy {
  display: grid;
  gap: 4px;
  min-width: 0;
}
.employee-copy strong {
  overflow: hidden;
  font-size: 13px;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.employee-copy small {
  overflow: hidden;
  color: var(--muted);
  font-size: 10px;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.state-chip,
.policy-chip {
  display: inline-flex;
  align-items: center;
  width: max-content;
  padding: 5px 8px;
  border: 1px solid #dbe5ef;
  border-radius: 7px;
  background: #f4f7fa;
  color: #60758c;
  font-size: 10px;
  font-weight: 800;
  white-space: nowrap;
}
.state-chip.success {
  border-color: #caeadf;
  background: #ecf9f4;
  color: var(--green);
}
.state-chip.warning {
  border-color: #f1ddb6;
  background: #fff7e9;
  color: var(--amber);
}
.state-chip.danger {
  border-color: #f3d1d1;
  background: #fff1f1;
  color: var(--red);
}
.policy-chip.direct {
  border-color: #caeadf;
  background: #ecf9f4;
  color: var(--green);
}
.policy-chip.approval_required {
  border-color: #f1ddb6;
  background: #fff7e9;
  color: var(--amber);
}

.panel__foot {
  padding: 11px 14px;
  border-top: 1px solid var(--line);
  color: var(--muted);
  font-size: 11px;
}
.panel__foot button {
  border: 0;
  background: transparent;
  color: var(--blue);
  font-size: 11px;
  font-weight: 700;
  cursor: pointer;
}
.panel__foot button:disabled {
  color: #b3c0cd;
  cursor: not-allowed;
}

.detail-panel {
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
}
.employee-summary {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 24px;
  padding: 18px 22px;
  border-bottom: 1px solid var(--line);
}
.summary-person {
  display: flex;
  align-items: center;
  gap: 12px;
}
.summary-title {
  display: flex;
  align-items: center;
  gap: 9px;
}
.summary-title h2 {
  font-size: 19px;
}
.summary-person p {
  margin-top: 5px;
  color: var(--muted);
  font-size: 11px;
}
.summary-facts {
  display: flex;
  gap: 22px;
}
.summary-facts span {
  display: grid;
  gap: 4px;
  min-width: 88px;
  font-size: 12px;
  font-weight: 700;
}
.summary-facts small {
  color: var(--muted);
  font-size: 9px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
}

.tabs {
  display: flex;
  gap: 4px;
  padding: 8px 18px 0;
  border-bottom: 1px solid var(--line);
}
.tabs button {
  display: flex;
  align-items: center;
  gap: 7px;
  height: 40px;
  padding: 0 14px;
  border: 0;
  border-bottom: 2px solid transparent;
  background: transparent;
  color: var(--muted);
  font-size: 12px;
  font-weight: 800;
  cursor: pointer;
}
.tabs button.active {
  border-bottom-color: var(--blue);
  color: var(--blue);
}
.tabs button span {
  padding: 2px 6px;
  border-radius: 999px;
  background: #edf2f7;
  font-size: 9px;
}
.tab-content {
  min-height: 0;
  overflow: auto;
  padding: 16px 18px 18px;
  background: #fbfcfe;
}
.employment-grid {
  display: grid;
  grid-template-columns: minmax(0, 1.35fr) minmax(280px, 0.85fr);
  gap: 14px;
}
.compensation-grid {
  display: grid;
  grid-template-columns: minmax(240px, 0.72fr) minmax(420px, 1.28fr);
  gap: 14px;
}
.form-card,
.lifecycle-card,
.proposal-list-card {
  min-height: 0;
  padding: 17px;
  border: 1px solid var(--line);
  border-radius: 13px;
  background: #fff;
}
.section-title {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}
.section-title h3 {
  margin-top: 4px;
  font-size: 16px;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-top: 16px;
}
.form-grid label,
.lifecycle-reason,
.reassign-box label {
  display: grid;
  gap: 6px;
}
.form-grid label > span,
.lifecycle-reason > span,
.reassign-box label > span {
  color: #5f748a;
  font-size: 10px;
  font-weight: 800;
}
.form-grid input,
.form-grid select,
.form-grid textarea,
.lifecycle-reason textarea,
.reassign-box input {
  width: 100%;
  border: 1px solid var(--line);
  border-radius: 9px;
  background: #fff;
  color: var(--ink);
  box-sizing: border-box;
  outline: none;
}
.form-grid input,
.form-grid select,
.reassign-box input {
  height: 39px;
  padding: 0 10px;
}
.form-grid textarea,
.lifecycle-reason textarea {
  min-height: 68px;
  padding: 10px;
  resize: none;
}
.form-grid input:focus,
.form-grid select:focus,
.form-grid textarea:focus {
  border-color: var(--blue);
  box-shadow: 0 0 0 3px var(--blue-soft);
}
.form-grid input:disabled,
.form-grid select:disabled,
.form-grid textarea:disabled {
  background: #f4f6f8;
  color: #8b9bad;
}
.span-2 {
  grid-column: 1 / -1;
}
.position-preview {
  display: grid;
  gap: 9px;
  margin-top: 13px;
  padding: 12px;
  border-radius: 10px;
  background: #f3f7fb;
}
.position-preview div {
  display: flex;
  justify-content: space-between;
  gap: 12px;
}
.position-preview strong {
  font-size: 12px;
}
.position-preview span,
.position-preview small {
  color: var(--muted);
  font-size: 10px;
}

.button {
  height: 39px;
  padding: 0 14px;
  border: 1px solid transparent;
  border-radius: 9px;
  background: #fff;
  font-size: 11px;
  font-weight: 800;
  cursor: pointer;
}
.button:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}
.button--wide {
  width: 100%;
  margin-top: 14px;
}
.button--primary {
  background: var(--blue);
  color: #fff;
}
.button--success {
  background: var(--green);
  color: #fff;
}
.button--danger {
  border-color: #efcaca;
  color: var(--red);
}
.button--ghost {
  border-color: var(--line);
  color: #425a72;
}
.text-button {
  border: 0;
  background: transparent;
  color: var(--blue);
  font-size: 11px;
  font-weight: 800;
  cursor: pointer;
}

.timeline {
  display: grid;
  gap: 0;
  margin: 18px 0;
  padding: 0;
  list-style: none;
}
.timeline li {
  position: relative;
  display: grid;
  grid-template-columns: 22px 1fr;
  gap: 9px;
  min-height: 54px;
  color: #8a9bac;
}
.timeline li::before {
  content: "";
  position: absolute;
  left: 7px;
  top: 17px;
  bottom: 0;
  width: 2px;
  background: #e1e8ef;
}
.timeline li:last-child::before {
  display: none;
}
.timeline li > span {
  position: relative;
  z-index: 1;
  width: 14px;
  height: 14px;
  margin-top: 2px;
  border: 3px solid #dfe7ef;
  border-radius: 50%;
  background: #fff;
  box-sizing: border-box;
}
.timeline li.done > span {
  border-color: var(--green);
  background: var(--green);
}
.timeline li.current > span {
  border-color: var(--blue);
}
.timeline li div {
  display: grid;
  align-content: start;
  gap: 4px;
}
.timeline strong {
  color: #425a72;
  font-size: 11px;
}
.timeline small {
  font-size: 9px;
}
.lifecycle-reason textarea {
  margin-top: 1px;
}
.lifecycle-actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 9px;
  margin-top: 11px;
}
.guard-note {
  margin-top: 11px;
  color: var(--muted);
  font-size: 9px;
  line-height: 1.6;
}
.guard-note.is-error {
  color: var(--red);
}

.proposal-list-card {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
}
.proposal-list {
  min-height: 0;
  margin-top: 12px;
  overflow: auto;
}
.proposal-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  width: 100%;
  padding: 11px 8px;
  border: 0;
  border-bottom: 1px solid #edf1f5;
  background: transparent;
  color: var(--ink);
  text-align: left;
  cursor: pointer;
}
.proposal-row.selected {
  background: var(--blue-soft);
}
.proposal-row > span:first-child {
  display: grid;
  gap: 4px;
}
.proposal-row strong {
  font-size: 11px;
}
.proposal-row small {
  color: var(--muted);
  font-size: 9px;
}
.proposal-editor {
  overflow: auto;
}
.proposal-actions {
  display: flex;
  gap: 9px;
  margin-top: 14px;
}
.proposal-actions .button {
  flex: 1;
}
.masked-note {
  margin-top: 13px;
  padding: 11px;
  border: 1px dashed #cbd8e5;
  border-radius: 9px;
  background: #f5f8fb;
  color: var(--muted);
  font-size: 10px;
  line-height: 1.6;
}
.reassign-box {
  display: grid;
  grid-template-columns: 1fr 1.2fr auto;
  align-items: end;
  gap: 9px;
  margin-top: 14px;
  padding-top: 14px;
  border-top: 1px solid var(--line);
}
.reassign-box button {
  height: 39px;
  border: 1px solid var(--line);
  border-radius: 9px;
  background: #fff;
  color: var(--blue);
  font-size: 10px;
  font-weight: 800;
  cursor: pointer;
}
.reassign-box button:disabled {
  opacity: 0.45;
}

.empty-state,
.empty-detail {
  display: grid;
  place-items: center;
  align-content: center;
  gap: 7px;
  min-height: 160px;
  padding: 24px;
  color: var(--muted);
  text-align: center;
}
.empty-state strong {
  color: #435b72;
  font-size: 12px;
}
.empty-state span {
  font-size: 10px;
}
.empty-state.compact {
  min-height: 130px;
  padding: 12px;
}
.empty-detail {
  grid-row: 1 / -1;
  min-height: 100%;
}
.empty-detail > span {
  display: grid;
  place-items: center;
  width: 48px;
  height: 48px;
  border: 1px solid #cfe0f0;
  border-radius: 15px;
  background: var(--blue-soft);
  color: var(--blue);
  font-weight: 900;
}
.empty-detail h2 {
  font-size: 18px;
}
.empty-detail p {
  max-width: 410px;
  font-size: 11px;
  line-height: 1.7;
}

.toast {
  position: fixed;
  right: 24px;
  bottom: 22px;
  z-index: 20;
  display: grid;
  gap: 3px;
  min-width: 270px;
  max-width: 420px;
  padding: 13px 42px 13px 15px;
  border: 1px solid #cbe8dd;
  border-radius: 11px;
  background: #effaf6;
  color: var(--green);
  box-shadow: 0 14px 34px rgba(29, 58, 88, 0.14);
}
.toast.error {
  border-color: #efcccc;
  background: #fff2f2;
  color: var(--red);
}
.toast strong {
  font-size: 11px;
}
.toast span {
  font-size: 10px;
}
.toast button {
  position: absolute;
  top: 8px;
  right: 10px;
  border: 0;
  background: transparent;
  color: inherit;
  font-size: 18px;
  cursor: pointer;
}

@media (max-width: 1150px) {
  .workspace__body {
    grid-template-columns: 300px minmax(0, 1fr);
  }
  .employee-summary {
    align-items: flex-start;
  }
  .summary-facts {
    display: none;
  }
  .employment-grid,
  .compensation-grid {
    grid-template-columns: 1fr;
  }
}
</style>
