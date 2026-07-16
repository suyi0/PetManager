<template>
  <section class="workspace">
    <header class="workspace__head">
      <div>
        <span class="eyebrow">POSITION ASSIGNMENT</span>
        <h1>职位任职</h1>
        <p>按部门职位派岗，发起入职、调岗、转正与离职流程。</p>
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

          <div class="tab-content employment-grid">
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
        </template>

        <div v-else class="empty-detail">
          <span>01</span>
          <h2>选择一名职工或候选人</h2>
          <p>左侧选择后，可按部门职位派岗，并发起转正或离职。</p>
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
import { getHttpErrorMessage } from "@/api/httpError";
import {
  DepartmentOption,
  PersonnelEmployee,
  PositionOption,
} from "@/shared/types/employmentWorkflow";
import { personnelApi } from "../../api/personnelApi";
import {
  assignmentPolicyLabel,
  employmentStatusLabel,
  initialOf,
  statusTone,
  today,
} from "../../utils/personnelLabels";

export default defineComponent({
  name: "PersonnelEmployment",
  setup() {
    const employees = ref<PersonnelEmployee[]>([]);
    const selectedEmployee = ref<PersonnelEmployee | null>(null);
    const departments = ref<DepartmentOption[]>([]);
    const positions = ref<PositionOption[]>([]);
    const keywordInput = ref("");
    const keyword = ref("");
    const page = ref(1);
    const pageSize = 12;
    const total = ref(0);
    const loadingEmployees = ref(false);
    const saving = ref(false);
    const lifecycleReason = ref("");
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
    const canRegularize = computed(
      () => selectedEmployee.value?.employmentStatus === "probation"
    );
    const canOffboard = computed(() =>
      ["probation", "regularization_pending", "active"].includes(
        selectedEmployee.value?.employmentStatus || ""
      )
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
      assignmentForm.departmentId = employee.departmentId ?? 0;
      assignmentForm.positionId = employee.positionId ?? 0;
      assignmentForm.reason = "";
      lifecycleReason.value = "";
      try {
        const detail = await personnelApi.getEmployee(employee.id);
        selectedEmployee.value = { ...employee, ...detail };
        assignmentForm.departmentId = detail.departmentId ?? 0;
        assignmentForm.positionId = detail.positionId ?? 0;
      } catch (error) {
        notify(getHttpErrorMessage(error, "职工详情加载失败"), "error");
      }
    };

    const refreshSelected = async () => {
      if (!selectedEmployee.value) return;
      const detail = await personnelApi.getEmployee(selectedEmployee.value.id);
      selectedEmployee.value = { ...selectedEmployee.value, ...detail };
      await loadEmployees();
    };

    const refreshAll = async () => {
      await Promise.all([loadEmployees(), loadOrg()]);
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
      try {
        await loadOrg();
      } catch (error) {
        notify(getHttpErrorMessage(error, "组织数据加载失败"), "error");
      }
      await loadEmployees();
    });

    return {
      applySearch,
      assignmentAction,
      assignmentForm,
      assignmentPolicyLabel,
      canOffboard,
      canRegularize,
      departments,
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
      refreshAll,
      regularizeEmployee,
      saveAssignment,
      saving,
      selectEmployee,
      selectedEmployee,
      selectedPosition,
      statusTone,
      total,
      totalPages,
    };
  },
});
</script>

<style src="./personnelWorkspace.css"></style>
