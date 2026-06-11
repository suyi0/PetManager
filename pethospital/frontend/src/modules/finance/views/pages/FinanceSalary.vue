<template>
  <section class="salary-page">
    <section class="salary-hero panel">
      <div class="salary-hero__copy">
        <p class="eyebrow">Payroll Control</p>
        <h3>员工工资管理台</h3>
        <span>
          聚合员工工资结构、当日成本与月度归档记录，支持按员工快速调整基础工资与绩效奖励。
        </span>
      </div>

      <div class="salary-hero__cards">
        <article class="metric-card">
          <small>在册员工</small>
          <strong>{{ summary.employeeCount }}</strong>
          <span>当前纳入工资管理的人数</span>
        </article>
        <article class="metric-card metric-card--accent">
          <small>月度工资池</small>
          <strong>{{ formatCurrency(summary.monthlyPayroll) }}</strong>
          <span>员工当前总工资汇总</span>
        </article>
        <article class="metric-card">
          <small>今日成本</small>
          <strong>{{ formatCurrency(summary.todayCost) }}</strong>
          <span>含工资摊销与当日药品成本</span>
        </article>
        <article class="metric-card metric-card--profit">
          <small>今日利润</small>
          <strong>{{ formatCurrency(summary.todayProfit) }}</strong>
          <span>营业额减去当日总成本</span>
        </article>
      </div>
    </section>

    <section class="salary-stage">
      <section class="salary-list panel">
        <div class="salary-list__head">
          <div>
            <p class="eyebrow">Employee Matrix</p>
            <h4>员工工资目录</h4>
          </div>
          <div class="salary-list__actions">
            <input
              v-model.trim="keyword"
              class="search-input"
              type="text"
              placeholder="搜索姓名 / 角色 / 手机号 / 邮箱"
            />
            <button class="ghost" type="button" @click="refreshSalaryData">
              刷新列表
            </button>
          </div>
        </div>

        <div class="salary-grid">
          <button
            v-for="employee in filteredEmployees"
            :key="employee.id"
            type="button"
            class="salary-card"
            :class="{
              'salary-card--active': selectedEmployee?.id === employee.id,
            }"
            @click="selectEmployee(employee.id)"
          >
            <div class="salary-card__head">
              <div>
                <small>#{{ employee.id }}</small>
                <strong>{{ employee.name || "未命名员工" }}</strong>
              </div>
              <span
                class="role-pill"
                :class="roleClassName(employee.type_name)"
              >
                {{ employee.type_name || "未知角色" }}
              </span>
            </div>
            <div class="salary-card__metrics">
              <article>
                <span>基础工资</span>
                <strong>{{ formatCurrency(employee.base_salary) }}</strong>
              </article>
              <article>
                <span>A 奖金</span>
                <strong>{{ formatCurrency(employee.pa_award) }}</strong>
              </article>
              <article>
                <span>B 奖金</span>
                <strong>{{ formatCurrency(employee.pb_award) }}</strong>
              </article>
            </div>
            <div class="salary-card__footer">
              <span>{{
                employee.email || employee.phone || "暂无联系方式"
              }}</span>
              <strong>{{ formatCurrency(employee.total_salary) }}</strong>
            </div>
          </button>

          <div v-if="filteredEmployees.length === 0" class="empty-card">
            当前没有符合条件的工资记录
          </div>
        </div>
      </section>

      <section class="salary-side">
        <section class="panel editor-panel">
          <div class="editor-panel__head">
            <div>
              <p class="eyebrow">Salary Editor</p>
              <h4>工资结构编辑</h4>
            </div>
            <button
              class="ghost"
              type="button"
              :disabled="!selectedEmployee"
              @click="resetEditor"
            >
              重置
            </button>
          </div>

          <div v-if="selectedEmployee" class="editor-panel__content">
            <div class="editor-profile">
              <strong>{{ selectedEmployee.name || "未命名员工" }}</strong>
              <span>
                {{ selectedEmployee.type_name || "未知角色" }} ·
                {{
                  selectedEmployee.phone ||
                  selectedEmployee.email ||
                  "暂无联系方式"
                }}
              </span>
              <small
                >最近更新：{{
                  selectedEmployee.updated_at || "暂无记录"
                }}</small
              >
            </div>

            <div class="editor-form">
              <label>
                <span>基础工资</span>
                <input
                  v-model.number="editor.baseSalary"
                  type="number"
                  min="0"
                  step="0.01"
                />
              </label>
              <label>
                <span>A 奖金</span>
                <input
                  v-model.number="editor.paAward"
                  type="number"
                  min="0"
                  step="0.01"
                />
              </label>
              <label>
                <span>B 奖金</span>
                <input
                  v-model.number="editor.pbAward"
                  type="number"
                  min="0"
                  step="0.01"
                />
              </label>
            </div>

            <div class="editor-total">
              <span>实时合计</span>
              <strong>{{ formatCurrency(editorTotal) }}</strong>
            </div>

            <p
              class="editor-message"
              :class="{
                'editor-message--error': statusMessageType === 'error',
              }"
            >
              {{ statusMessage }}
            </p>

            <button
              class="primary"
              type="button"
              :disabled="saving"
              @click="saveSalary"
            >
              {{ saving ? "保存中..." : "保存工资结构" }}
            </button>
          </div>

          <div v-else class="empty-card empty-card--side">
            请选择左侧员工卡片后再编辑工资
          </div>
        </section>

        <section class="panel records-panel">
          <div class="records-panel__head">
            <div>
              <p class="eyebrow">Archive</p>
              <h4>工资记录归档</h4>
            </div>
          </div>

          <div class="records-group">
            <h5>月度汇总</h5>
            <article
              v-for="record in salaryManagement.monthlyRecords"
              :key="`month-${record.id}`"
              class="record-item"
            >
              <span>{{ formatRecordDate(record.created_at) }}</span>
              <strong>{{ formatCurrency(record.profitCount) }}</strong>
              <small>
                营收 {{ formatCurrency(record.salesCount) }} / 成本
                {{ formatCurrency(record.costCount) }}
              </small>
            </article>
            <div
              v-if="salaryManagement.monthlyRecords.length === 0"
              class="record-empty"
            >
              暂无月度工资归档
            </div>
          </div>

          <div class="records-group">
            <h5>近 10 日明细</h5>
            <article
              v-for="record in salaryManagement.dailyRecords"
              :key="`day-${record.id}`"
              class="record-item"
            >
              <span>{{ formatRecordDate(record.created_at) }}</span>
              <strong>{{ formatCurrency(record.costCount) }}</strong>
              <small>
                营收 {{ formatCurrency(record.salesCount) }} / 利润
                {{ formatCurrency(record.profitCount) }}
              </small>
            </article>
            <div
              v-if="salaryManagement.dailyRecords.length === 0"
              class="record-empty"
            >
              暂无日度工资明细
            </div>
          </div>
        </section>
      </section>
    </section>
  </section>
</template>

<script lang="ts">
import {
  computed,
  defineComponent,
  onBeforeUnmount,
  onMounted,
  reactive,
  ref,
  watch,
} from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { useRoute } from "vue-router";
import { financeApi } from "../../api/financeApi";
import { SalaryEmployeeRow } from "../../api/types";
import { isSuperAdminPortalRole } from "@/core/auth/utils/roleUtils";
import { saveFinanceHomeDataCache } from "../../utils/financeDataCache";
import { subscribeFinanceHomeData } from "../../utils/financeHomeDataStream";

export default defineComponent({
  name: "FinanceSalary",
  setup() {
    const store = useStore(storeKey);
    const route = useRoute();
    let closeHomeDataStream: (() => void) | null = null;
    const keyword = ref("");
    const selectedEmployeeId = ref<number | null>(null);
    const saving = ref(false);
    const statusMessage = ref("选择员工后可调整工资结构");
    const statusMessageType = ref<"info" | "error">("info");
    const editor = reactive({
      baseSalary: 0,
      paAward: 0,
      pbAward: 0,
    });

    const salaryManagement = computed(
      () => store.state.finance.salaryManagement
    );
    const homeData = computed(() => store.state.finance.homeData);
    const isFinanceHomePage = computed(() => route.name === "financeSalary");
    const summary = computed(() => salaryManagement.value.summary);
    const realtimeSummary = computed(() => ({
      ...summary.value,
      todayCost: homeData.value.dailyCost || summary.value.todayCost,
      todayProfit: homeData.value.dailyProfit || summary.value.todayProfit,
    }));
    const employees = computed(() => salaryManagement.value.employees);

    const filteredEmployees = computed(() => {
      const search = keyword.value.trim().toLowerCase();
      if (!search) {
        return employees.value;
      }

      return employees.value.filter((item) =>
        [item.name, item.type_name, item.phone, item.email]
          .filter((field): field is string => typeof field === "string")
          .some((field) => field.toLowerCase().includes(search))
      );
    });

    const selectedEmployee = computed<SalaryEmployeeRow | null>(() => {
      if (selectedEmployeeId.value === null) {
        return filteredEmployees.value[0] ?? employees.value[0] ?? null;
      }

      return (
        employees.value.find((item) => item.id === selectedEmployeeId.value) ??
        null
      );
    });

    const editorTotal = computed(
      () =>
        Number(editor.baseSalary || 0) +
        Number(editor.paAward || 0) +
        Number(editor.pbAward || 0)
    );

    const formatCurrency = (value: number) =>
      `￥${Number(value || 0).toFixed(2)}`;

    const formatRecordDate = (value: string) =>
      value ? value.replace("T", " ").slice(0, 16) : "暂无日期";

    const roleClassName = (role?: string) => {
      if (role && isSuperAdminPortalRole(role)) {
        return "role-pill--super";
      }
      if (role === "医生" || role === "护士") {
        return "role-pill--medical";
      }
      if (role === "仓库管理员") {
        return "role-pill--warehouse";
      }
      return "role-pill--staff";
    };

    const syncEditor = (employee: SalaryEmployeeRow | null) => {
      editor.baseSalary = Number(employee?.base_salary ?? 0);
      editor.paAward = Number(employee?.pa_award ?? 0);
      editor.pbAward = Number(employee?.pb_award ?? 0);
    };

    watch(
      selectedEmployee,
      (employee) => {
        if (!employee) {
          return;
        }

        selectedEmployeeId.value = employee.id;
        syncEditor(employee);
      },
      { immediate: true }
    );

    const ensureSalaryData = async () => {
      await store.dispatch("finance/ensureHomeData");
      await store.dispatch("finance/ensureSalaryManagement");
      if (!selectedEmployeeId.value && employees.value[0]) {
        selectedEmployeeId.value = employees.value[0].id;
      }
    };

    const refreshSalaryData = async () => {
      await store.dispatch("finance/refreshSalaryManagement");
      statusMessage.value = "工资数据已刷新";
      statusMessageType.value = "info";
      if (!selectedEmployeeId.value && employees.value[0]) {
        selectedEmployeeId.value = employees.value[0].id;
      }
    };

    const selectEmployee = (userId: number) => {
      selectedEmployeeId.value = userId;
      statusMessage.value = `已选择员工 #${userId}`;
      statusMessageType.value = "info";
    };

    const resetEditor = () => {
      syncEditor(selectedEmployee.value);
      statusMessage.value = "已恢复到最近一次保存的工资结构";
      statusMessageType.value = "info";
    };

    const saveSalary = async () => {
      if (!selectedEmployee.value) {
        statusMessage.value = "请先选择员工";
        statusMessageType.value = "error";
        return;
      }

      saving.value = true;
      try {
        await financeApi.changeSalary({
          userId: selectedEmployee.value.id,
          baseSalary: Number(editor.baseSalary || 0),
          paAward: Number(editor.paAward || 0),
          pbAward: Number(editor.pbAward || 0),
        });
        store.commit("finance/markSalaryManagementDirty");
        statusMessage.value = "工资结构保存成功";
        statusMessageType.value = "info";
        await store.dispatch("finance/refreshSalaryManagement");
      } catch (error: unknown) {
        statusMessage.value = `工资保存失败: ${String(
          (error as Error).message || error
        )}`;
        statusMessageType.value = "error";
      } finally {
        saving.value = false;
      }
    };

    onMounted(() => {
      void ensureSalaryData();

      if (isFinanceHomePage.value) {
        closeHomeDataStream = subscribeFinanceHomeData(
          (nextHomeData) => {
            store.commit("finance/setHomeData", nextHomeData);
            saveFinanceHomeDataCache(nextHomeData);
          },
          {
            onFallbackRefresh: () => {
              void store.dispatch("finance/refreshHomeData");
            },
          }
        );
      }
    });

    onBeforeUnmount(() => {
      closeHomeDataStream?.();
      closeHomeDataStream = null;
    });

    return {
      isFinanceHomePage,
      keyword,
      summary: realtimeSummary,
      salaryManagement,
      filteredEmployees,
      selectedEmployee,
      editor,
      editorTotal,
      saving,
      statusMessage,
      statusMessageType,
      formatCurrency,
      formatRecordDate,
      roleClassName,
      refreshSalaryData,
      selectEmployee,
      resetEditor,
      saveSalary,
    };
  },
});
</script>

<style scoped>
.salary-page {
  display: grid;
  gap: 18px;
}

.panel {
  border: 1px solid #dce7ff;
  border-radius: 24px;
  background: radial-gradient(
      circle at top left,
      rgba(47, 111, 243, 0.11),
      transparent 30%
    ),
    linear-gradient(180deg, #ffffff 0%, #f6f9ff 100%);
  box-shadow: 0 22px 44px rgba(34, 64, 128, 0.08);
}

.eyebrow {
  margin: 0 0 8px;
  color: #6c7a9f;
  font-size: 12px;
  letter-spacing: 0.24em;
  text-transform: uppercase;
}

.salary-hero {
  display: grid;
  gap: 22px;
  padding: 24px;
}

.salary-hero__copy h3,
.salary-list__head h4,
.editor-panel__head h4,
.records-panel__head h4 {
  margin: 0;
  color: #16294d;
}

.salary-hero__copy span {
  color: #66789a;
  line-height: 1.8;
}

.salary-hero__cards {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
}

.metric-card {
  display: grid;
  gap: 6px;
  padding: 18px;
  border-radius: 18px;
  border: 1px solid #d9e5ff;
  background: rgba(255, 255, 255, 0.8);
}

.metric-card small {
  color: #7486a6;
}

.metric-card strong {
  color: #1f3e68;
  font-size: 28px;
}

.metric-card span {
  color: #7d8eab;
  font-size: 13px;
}

.metric-card--accent {
  background: linear-gradient(135deg, #ebf5ff, #f4f9ff);
}

.metric-card--profit {
  background: linear-gradient(135deg, #eefcf2, #f8fff9);
}

.salary-stage {
  display: grid;
  grid-template-columns: minmax(0, 1.35fr) minmax(360px, 0.9fr);
  gap: 18px;
}

.salary-list,
.salary-side {
  display: grid;
  gap: 18px;
}

.salary-list {
  padding: 20px;
}

.salary-list__head,
.editor-panel__head,
.records-panel__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;
}

.salary-list__actions {
  display: flex;
  gap: 12px;
  align-items: center;
}

.search-input,
.editor-form input {
  border: 1px solid #d2ddf4;
  border-radius: 14px;
  padding: 11px 14px;
  font-size: 14px;
  background: rgba(255, 255, 255, 0.92);
  color: #26415f;
}

.search-input {
  min-width: 280px;
}

button {
  border: 0;
  border-radius: 14px;
  cursor: pointer;
}

.ghost,
.primary {
  padding: 11px 16px;
  font-weight: 700;
}

.ghost {
  background: #eaf1ff;
  color: #2f6ff3;
}

.primary {
  background: linear-gradient(135deg, #2f6ff3, #4bb4ff);
  color: #fff;
}

.salary-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.salary-card {
  display: grid;
  gap: 14px;
  padding: 18px;
  border: 1px solid #dce7ff;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.9);
  color: #1d3158;
  text-align: left;
  box-shadow: 0 16px 32px rgba(49, 81, 146, 0.08);
  transition: transform 0.18s ease, box-shadow 0.18s ease,
    border-color 0.18s ease;
}

.salary-card:hover {
  transform: translateY(-1px);
  box-shadow: 0 20px 36px rgba(49, 81, 146, 0.12);
}

.salary-card--active {
  border-color: #81a9ff;
  background: linear-gradient(180deg, #f0f5ff, #ebf2ff);
}

.salary-card__head,
.salary-card__footer {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: center;
}

.salary-card__head small,
.salary-card__footer span,
.editor-profile span,
.editor-profile small,
.record-item small,
.record-empty {
  color: #7284a5;
}

.salary-card__head strong,
.salary-card__footer strong,
.editor-total strong {
  color: #17345f;
}

.salary-card__metrics {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px;
}

.salary-card__metrics article,
.record-item {
  display: grid;
  gap: 4px;
  padding: 12px 14px;
  border-radius: 14px;
  background: #f6f9ff;
}

.salary-card__metrics span,
.record-item span,
.editor-total span,
.editor-form label span {
  color: #6d809f;
  font-size: 12px;
}

.salary-card__metrics strong,
.record-item strong {
  font-size: 18px;
}

.editor-panel,
.records-panel {
  padding: 20px;
}

.editor-panel__content,
.records-group,
.editor-form,
.editor-profile {
  display: grid;
  gap: 12px;
}

.editor-form {
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.editor-form label {
  display: grid;
  gap: 8px;
}

.editor-total {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 14px 16px;
  border-radius: 16px;
  background: linear-gradient(135deg, #eef5ff, #f7fbff);
}

.editor-message {
  margin: 0;
  color: #4b658f;
}

.editor-message--error {
  color: #c24f4f;
}

.role-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 88px;
  padding: 6px 10px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.role-pill--super {
  background: #fff0da;
  color: #b56a00;
}

.role-pill--medical {
  background: #dff5eb;
  color: #1f8a61;
}

.role-pill--warehouse {
  background: #fff1d8;
  color: #b57400;
}

.role-pill--staff {
  background: #edf2ff;
  color: #4f67b5;
}

.empty-card,
.record-empty {
  display: grid;
  place-items: center;
  min-height: 140px;
  border: 1px dashed #d7e3fb;
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.7);
  color: #6f82a5;
  text-align: center;
}

.empty-card--side {
  min-height: 220px;
}

@media (max-width: 1200px) {
  .salary-stage {
    grid-template-columns: 1fr;
  }

  .salary-hero__cards,
  .salary-grid,
  .editor-form {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 820px) {
  .salary-list__head,
  .salary-list__actions,
  .salary-hero__cards,
  .salary-grid,
  .editor-form {
    grid-template-columns: 1fr;
  }

  .salary-list__actions {
    display: grid;
  }

  .search-input {
    min-width: 0;
  }
}
</style>
