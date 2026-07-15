<template>
  <section class="payroll-page">
    <header class="payroll-header panel-dark">
      <div>
        <h2>{{ periodLabel }}薪资核算周期</h2>
        <p v-if="canReadSalary">预计发放人数：{{ period.employeeCount }} 人 · 当前版本：v{{ period.versionNo }} · 状态：{{ periodStatusLabel }}</p>
        <p v-else>当前账号没有工资数据查看权限</p>
      </div>
      <div class="header-actions">
        <button class="dark-button" type="button" :disabled="!canOpenReviewCenter" @click="reviewDrawerOpen = true">复审中心</button>
        <button class="dark-button" type="button" :disabled="!canActivateSalaryProfile" @click="openCompensationQueue">薪资激活队列</button>
        <button class="light-button" type="button" :disabled="!canSubmitReview" @click="submitReview">提交主管复审</button>
      </div>
    </header>

    <section v-if="!canReadSalary" class="panel access-denied">
      <h3>无法查看工资数据</h3>
      <p>当前账号仅可进入财务门户，未授予 salary:read。请由权限管理员按岗位授权后重试。</p>
    </section>

    <template v-else>

    <section class="progress-track panel">
      <div v-for="step in workflowSteps" :key="step.key" class="progress-step" :class="step.state">
        <span class="progress-dot">{{ step.state === 'done' ? '✓' : step.index }}</span>
        <span>{{ step.label }}</span>
      </div>
    </section>

    <section class="summary-grid" :class="{ comfortable: summaryComfortable }">
      <article v-for="metric in summaryMetrics" :key="metric.label" class="summary-card" :class="metric.tone">
        <span>{{ metric.label }}</span>
        <strong>{{ metric.value }}</strong>
      </article>
    </section>

    <section class="payroll-layout">
      <main class="ledger-panel panel">
        <div class="section-heading">
          <div>
            <h3>{{ periodLabel }}工资台账</h3>
          </div>
          <div class="toolbar">
            <input v-model.trim="keywordInput" placeholder="搜索员工 / 工号 / 职位" @keyup.enter="refreshEmployees" />
            <select v-model="payTypeFilter" @change="refreshEmployees"><option value="all">全部工种</option><option value="monthly">月薪</option><option value="hourly">时薪</option></select>
            <select v-model="modifiedFilter" @change="refreshEmployees"><option value="all">全部记录</option><option value="modified">人工改动</option><option value="clean">系统生成</option></select>
            <button class="outline-button" type="button" @click="refreshEmployees">刷新核算</button>
          </div>
        </div>

        <div class="ledger-scroll" :class="{ 'is-empty': visibleEmployees.length === 0 }">
          <div class="ledger ledger-head">
            <span>员工 / 岗位</span><span>薪资标准</span><span class="col-r">工时/月</span><span class="col-r">全勤奖</span><span class="col-r">绩效奖</span><span class="col-r">补贴</span><span class="col-r">扣款/五险</span><span class="col-r">实发工资</span><span>数据类型</span><span>初审状态</span>
          </div>
          <button v-for="employee in visibleEmployees" :key="employee.id" class="ledger ledger-row" :class="{ selected: selectedEmployee?.id === employee.id, blocked: employee.review_status === 'blocked' }" type="button" @click="selectEmployee(employee)">
            <span class="employee-cell"><b>{{ employee.name || '未命名员工' }}</b><small>{{ employee.type_name || '未分配职位' }} · {{ employee.pay_type === 'hourly' ? '时薪' : '月薪' }}</small></span>
            <span class="mono">{{ standardLabel(employee) }}</span><span class="mono col-r">{{ brief(employee.work_hours_month) }}h</span>
            <span class="mono col-r positive">+{{ money(employee.attendance_award) }}</span>
            <span class="mono col-r positive">+{{ money(employee.performance_award) }}</span>
            <span class="mono col-r positive">+{{ money(employee.allowance) }}</span>
            <span class="mono col-r negative">−{{ money(deductions(employee)) }}</span>
            <strong class="mono total col-r">{{ money(employee.total_salary) }}</strong>
            <span><em v-if="employee.is_manually_modified" class="modified-mark">人工改动 {{ employee.change_count || 1 }}</em><small v-else class="system-mark">系统生成</small></span>
            <span><em class="status-mark" :class="employee.review_status">{{ reviewStatusLabel(employee.review_status) }}</em></span>
          </button>
          <div v-if="visibleEmployees.length === 0" class="empty-state">当前周期暂无工资快照，请先设置薪资配置并生成周期。</div>
        </div>

        <footer class="ledger-foot">
          <AppPager :page="currentPage" :total-pages="totalPages" :total-items="totalEmployees" :page-size="uiPageSize" @update:page="goToPage" />
        </footer>
      </main>

    </section>

    <!-- 工资明细子界面 - 侧边滑出抽屉 -->
    <div v-if="detailDrawerOpen && selectedEmployee" class="drawer-backdrop" @click.self="detailDrawerOpen = false">
      <aside class="detail-panel panel detail-drawer">
        <button class="drawer-close" type="button" @click="detailDrawerOpen = false">×</button>
        <div class="detail-head">
          <div>
            <h3>{{ selectedEmployee.name }} · 工资明细</h3>
            <p>salary #{{ selectedEmployee.salary_id || '待生成' }} · salaryProfile #{{ selectedEmployee.salary_profile_id || '未配置' }} · {{ periodLabel }} v{{ period.versionNo }}</p>
          </div>
          <button class="outline-button" type="button" @click="printEmployee(selectedEmployee)">打印工资单</button>
        </div>
        <div class="detail-cards">
          <article>
            <span>计薪公式</span>
            <b>{{ formula(selectedEmployee) }}</b>
            <strong class="total-salary">{{ money(selectedEmployee.total_salary) }}</strong>
          </article>
          <article>
            <span>工作时间与系统计算</span>
            <b>{{ money(selectedEmployee.work_hours_month) }} 小时 · 全勤 {{ selectedEmployee.attendance_days || 0 }} 天</b>
            <small>绩效奖金规则按服务对象数量逐步接入</small>
          </article>
          <article class="change-card">
            <span>人工改动标志</span>
            <b>{{ selectedEmployee.is_manually_modified ? `已修改 ${selectedEmployee.change_count || 1} 个字段` : '系统生成' }}</b>
            <small>点击“查看凭证”读取 salaryChangeRecord</small>
          </article>
        </div>
        <div class="change-history">
          <span class="label">修改记录</span>
          <button v-if="selectedEmployee.is_manually_modified" class="history-row" type="button" @click="toggleHistory">
            <b>{{ historyLoading ? '读取中…' : '查看字段级修改凭证' }}</b>
            <span>操作人、原因、前后值、附件 →</span>
          </button>
          <p v-else class="history-empty">当前工资快照没有人工修改记录。</p>
          <div v-if="showHistory" class="history-expanded">
            <p v-if="historyError">{{ historyError }}</p>
            <p v-else-if="history.length === 0">暂无变更记录。</p>
            <div v-for="record in history" v-else :key="record.id" class="history-record">
              {{ record.changed_field }}：{{ record.before_value || '空' }} → {{ record.after_value || '空' }} · {{ record.changed_by_name || '未知操作人' }} · {{ record.created_at }}
            </div>
          </div>
        </div>
        <div class="drawer-actions">
          <button v-if="canReviewSalary && canEditSnapshot && (selectedEmployee.review_status === 'pending' || selectedEmployee.review_status === 'returned')" class="success-button" type="button" :disabled="saving" @click="reviewSelectedAndClose">完成本人工资初审</button>
          <button v-if="canWriteSalary && canEditSnapshot" class="primary-button" type="button" @click="openEditDrawer">编辑本人工资快照</button>
        </div>
      </aside>
    </div>

    <div v-if="reviewDrawerOpen" class="drawer-backdrop" @click.self="reviewDrawerOpen = false">
      <aside class="review-panel panel review-drawer">
        <button class="drawer-close" type="button" @click="reviewDrawerOpen = false">×</button>
        <h3>复审中心</h3>
        <p>服务器状态：{{ periodStatusLabel }} · row v{{ period.rowVersion }} · 业务 v{{ period.versionNo }}</p>
        <article class="check-item" :class="statusTone"><b>{{ periodStatusLabel }}</b><small>{{ statusHint }}</small></article>
        <span class="label">周期检查</span>
        <article class="check-item done"><b>✓　工资快照已生成</b><small>{{ period.employeeCount }} 名员工，版本 v{{ period.versionNo }}</small></article>
        <article class="check-item danger"><b>!　{{ period.unconfiguredCount }} 人缺少生效薪资配置</b><small>阻断提交复审</small><button type="button" @click="openCompensationQueue">处理 →</button></article>
        <article class="check-item warning"><b>!　{{ period.modifiedCount }} 人存在人工改动</b><small>复审时逐项查看前后值和凭证</small><button type="button" @click="modifiedFilter = 'modified'">查看 →</button></article>
        <article class="check-item neutral"><b>!　{{ period.reviewedCount }} / {{ period.employeeCount }} 已完成初审</b><small>全部 first_reviewed 后才能提交</small></article>
        <article v-if="period.returnedCount > 0" class="check-item danger"><b>!　{{ period.returnedCount }} 人被主管退回</b><small>{{ period.supervisorNote || '请按退回意见整改后重审' }}</small></article>
        <template v-if="period.submittedAt"><span class="label">提交信息</span><p class="meta-line">提交人 #{{ period.submittedBy }} · {{ period.submittedAt }}</p></template>
        <template v-if="period.supervisorDecision"><span class="label">主管决定</span><p class="meta-line">{{ period.supervisorDecision === 'approve' ? '已批准' : '已退回' }} · #{{ period.supervisorReviewedBy }} · {{ period.supervisorReviewedAt }}</p><p v-if="period.supervisorNote" class="meta-line">意见：{{ period.supervisorNote }}</p></template>
        <span class="label">财务提交说明</span>
        <textarea v-model.trim="reviewNote" :disabled="!canSubmitReview" placeholder="可选：说明提交依据、异常处理或凭证编号" />
        <template v-if="canSupervisorReview && period.status === 'submitted_for_supervisor'">
          <span class="label">主管复审意见</span>
          <textarea v-model.trim="supervisorNote" placeholder="批准可选；退回必填" />
          <label class="checkbox-row"><input v-model="returnAll" type="checkbox" /> 退回时标记全部已初审工资行</label>
        </template>
        <p v-if="actionError" class="action-error">{{ actionError }}</p>
        <div class="lock-note"><b>锁定规则</b><p>锁定后 salary 快照只读；错误需创建同月 v2。提交人不能自己批准。批准后才能锁定。</p></div>
        <button v-if="canSubmitReviewPermission" class="muted-wide" type="button" :disabled="!canSubmitReview || saving" @click="submitReview">提交主管复审</button>
        <button v-if="canSupervisorReview && period.status === 'submitted_for_supervisor'" class="success-button wide" type="button" :disabled="saving" @click="approvePeriod">批准整周期</button>
        <button v-if="canSupervisorReview && period.status === 'submitted_for_supervisor'" class="danger-button wide" type="button" :disabled="saving || !supervisorNote" @click="returnPeriod">退回修改</button>
        <button v-if="canLockSalary" class="muted-wide" type="button" :disabled="period.status !== 'second_review' || saving" @click="lockPeriod">锁定工资周期</button>
        <button v-if="canCreateRevision" class="muted-wide" type="button" :disabled="saving" @click="createRevision">创建 v{{ period.versionNo + 1 }} 修订版</button>
      </aside>
    </div>

    <div v-if="editDrawerOpen" class="drawer-backdrop" @click.self="closeDrawers">
      <aside class="drawer panel">
        <button class="drawer-close" type="button" @click="closeDrawers">×</button>
        <span class="kicker">PAYROLL SNAPSHOT</span><h3>{{ selectedEmployee?.name }} · 快照编辑</h3><p>修改已有记录必须填写原因，并写入 salaryChangeRecord。</p><label>工作时间 / 月<input v-model.number="editForm.work_hours_month" type="number" min="0" /></label><label>全勤奖金<input v-model.number="editForm.attendance_award" type="number" min="0" /></label><label>绩效奖金<input v-model.number="editForm.performance_award" type="number" min="0" /></label><label>补贴<input v-model.number="editForm.allowance" type="number" min="0" /></label><label>扣款<input v-model.number="editForm.deduction" type="number" min="0" /></label><label>五险一金<input v-model.number="editForm.social_insurance_housing_fund" type="number" min="0" /></label><label>修改原因<textarea v-model.trim="editForm.change_reason" placeholder="必填：说明数据来源或凭证编号" /></label><button class="primary-button" type="button" :disabled="saving" @click="savePayrollRow">保存并记录修改</button>
      </aside>
    </div>
    </template>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { AppHttpError, getHttpErrorMessage } from "@/api/httpError";
import { financeApi } from "../../api/financeApi";
import { SalaryChangeRecord, SalaryEmployeeRow } from "../../api/types";
import AppPager from "@/shared/components/AppPager.vue";

export default defineComponent({
  name: "FinanceSalary",
  components: { AppPager },
  setup() {
    const uiPageSize = 5;
    const fetchPageSize = uiPageSize * 2;
    const store = useStore(storeKey);
    const router = useRouter();
    const employees = ref<SalaryEmployeeRow[]>([]);
    const currentPage = ref(1);
    const totalEmployees = ref(0);
    const pageChunks = new Map<number, SalaryEmployeeRow[]>();
    const keywordInput = ref("");
    const payTypeFilter = ref("all");
    const modifiedFilter = ref("all");
    const selectedEmployee = ref<SalaryEmployeeRow | null>(null);
    const editDrawerOpen = ref(false);
    const reviewDrawerOpen = ref(false);
    const detailDrawerOpen = ref(false); // 侧拉明细抽屉状态
    const summaryComfortable = ref(false);
    const showHistory = ref(false);
    const history = ref<SalaryChangeRecord[]>([]);
    const historyLoading = ref(false);
    const historyError = ref("");
    const reviewNote = ref("");
    const supervisorNote = ref("");
    const returnAll = ref(true);
    const actionError = ref("");
    const saving = ref(false);
    const period = reactive({
      status: "first_review",
      versionNo: 1,
      rowVersion: 1,
      employeeCount: 0,
      unconfiguredCount: 0,
      modifiedCount: 0,
      reviewedCount: 0,
      returnedCount: 0,
      submittedBy: 0,
      submittedAt: "",
      supervisorReviewedBy: 0,
      supervisorReviewedAt: "",
      supervisorDecision: "",
      supervisorNote: "",
    });
    const editForm = reactive({ work_hours_month: 0, attendance_award: 0, performance_award: 0, allowance: 0, deduction: 0, social_insurance_housing_fund: 0, change_reason: "" });
    const periodLabel = computed(() => new Date().toLocaleDateString("zh-CN", { year: "numeric", month: "long" }));
    const periodStatusLabel = computed(() => ({
      calculating: "生成中",
      first_review: "财务初审",
      submitted_for_supervisor: "待主管处理",
      second_review: "主管已批准",
      correction_required: "已退回修改",
      locked: "已锁定",
      archived: "已归档",
    }[period.status] || "生成中"));
    const statusTone = computed(() => ({
      first_review: "neutral",
      submitted_for_supervisor: "warning",
      second_review: "done",
      correction_required: "danger",
      locked: "done",
      archived: "done",
    }[period.status] || "neutral"));
    const statusHint = computed(() => ({
      first_review: "财务完成全部初审后可提交主管",
      submitted_for_supervisor: "工资快照已冻结，等待主管批准或退回",
      second_review: "主管已批准完整周期，可执行锁定",
      correction_required: "请按意见整改、重审后再次提交",
      locked: "快照只读；错误请创建修订版",
      archived: "历史归档",
    }[period.status] || ""));
    const hasPermission = (permission: string) => store.state.auth.permissions.includes(permission);
    const canReadSalary = computed(() => hasPermission("salary:read"));
    const canWriteSalary = computed(() => hasPermission("salary:write"));
    const canActivateSalaryProfile = computed(() => hasPermission("salary-profile:activate"));
    const canReviewSalary = computed(() => hasPermission("salary:review"));
    const canSubmitReviewPermission = computed(() => hasPermission("salary:submit-review"));
    const canSupervisorReview = computed(() => hasPermission("salary:supervisor-review"));
    const canLockSalary = computed(() => hasPermission("salary:lock"));
    const canOpenReviewCenter = computed(() => canSubmitReviewPermission.value || canSupervisorReview.value || canLockSalary.value || canReviewSalary.value);
    const canEditSnapshot = computed(() => period.status === "first_review" || period.status === "correction_required");
    const canCreateRevision = computed(() => canWriteSalary.value && (period.status === "locked" || period.status === "archived"));
    const totalPages = computed(() => Math.max(1, Math.ceil(totalEmployees.value / uiPageSize)));
    const currentChunkRows = computed(() => employees.value.slice(((currentPage.value - 1) % 2) * uiPageSize, ((currentPage.value - 1) % 2 + 1) * uiPageSize));
    const visibleEmployees = computed(() => currentChunkRows.value.filter((item) => (payTypeFilter.value === "all" || item.pay_type === payTypeFilter.value) && (modifiedFilter.value === "all" || (modifiedFilter.value === "modified" ? item.is_manually_modified : !item.is_manually_modified))));
    const canSubmitReview = computed(() =>
      canSubmitReviewPermission.value &&
      (period.status === "first_review" || period.status === "correction_required") &&
      period.unconfiguredCount === 0 &&
      period.reviewedCount === period.employeeCount &&
      period.employeeCount > 0
    );
    const workflowSteps = computed(() => {
      const order = ["first_review", "submitted_for_supervisor", "second_review", "locked", "archived"];
      const statusForProgress = period.status === "correction_required" ? "first_review" : period.status;
      const activeIdx = Math.max(0, order.indexOf(statusForProgress));
      return [
        { index: 1, key: "generated", label: "生成月度快照", state: "done" },
        { index: 2, key: "first_review", label: period.status === "correction_required" ? "整改重审" : "财务初审", state: activeIdx === 0 ? "active" : activeIdx > 0 ? "done" : "idle" },
        { index: 3, key: "submitted_for_supervisor", label: "待主管处理", state: activeIdx === 1 ? "active" : activeIdx > 1 ? "done" : "idle" },
        { index: 4, key: "second_review", label: "主管已批准", state: activeIdx === 2 ? "active" : activeIdx > 2 ? "done" : "idle" },
        { index: 5, key: "locked", label: "版本锁定", state: activeIdx >= 3 && period.status === "locked" ? "active" : activeIdx > 3 ? "done" : "idle" },
      ];
    });
    const summaryMetrics = computed(() => {
      const metrics = [
        { label: "预计实发", value: money(employees.value.reduce((sum, row) => sum + Number(row.total_salary || 0), 0)), tone: "neutral" },
        { label: "全勤奖金", value: `+${money(employees.value.reduce((sum, row) => sum + Number(row.attendance_award || 0), 0))}`, tone: "positive" },
        { label: "绩效奖金", value: `+${money(employees.value.reduce((sum, row) => sum + Number(row.performance_award || 0), 0))}`, tone: "positive" },
        { label: "补贴", value: `+${money(employees.value.reduce((sum, row) => sum + Number(row.allowance || 0), 0))}`, tone: "neutral" },
        { label: "扣款与五险一金", value: `-${money(employees.value.reduce((sum, row) => sum + Number(row.deduction || 0) + Number(row.social_insurance_housing_fund || 0), 0))}`, tone: "negative" },
      ];
      // 仅总院（scope:all）后端才返回非零：未归属到任何部门的当日营收，供「各部门之和 + 未归属 = 全院」对账。
      const unassigned = Number(store.state.finance.homeData.dailyUnassignedSales || 0);
      if (unassigned > 0) {
        metrics.push({ label: "未归属营收（今日·全院对账）", value: money(unassigned), tone: "neutral" });
      }
      return metrics;
    });
    const money = (value: number | null | undefined) => `￥${Number(value || 0).toFixed(2)}`;
    const brief = (value: number | null | undefined) => { const n = Number(value || 0); return Number.isInteger(n) ? n.toLocaleString("en-US") : n.toFixed(2); };
    const additions = (row: SalaryEmployeeRow) => Number(row.attendance_award || 0) + Number(row.performance_award || 0) + Number(row.allowance || 0);
    const deductions = (row: SalaryEmployeeRow) => Number(row.deduction || 0) + Number(row.social_insurance_housing_fund || 0);
    const standardLabel = (row: SalaryEmployeeRow) => row.pay_type === "hourly" ? `${money(row.hourly_rate)}/小时` : `${money(row.base_salary)}/月`;
    const reviewStatusLabel = (status?: string) => ({ pending: "待初审", first_reviewed: "已初审", returned: "已退回", second_reviewed: "历史已复审", locked: "已锁定", blocked: "需核对" }[status || "pending"] || "待初审");
    const formula = (row: SalaryEmployeeRow) => row.pay_type === "hourly" ? `${money(row.hourly_rate)} × ${row.work_hours_month || 0}h + 加项 - 扣项` : `${money(row.base_salary)} + 加项 - 扣项`;

    const selectEmployee = (row: SalaryEmployeeRow) => {
      selectedEmployee.value = row;
      showHistory.value = false;
      history.value = [];
      historyError.value = "";
      detailDrawerOpen.value = true;
    };

    const toggleHistory = async () => {
      if (!selectedEmployee.value?.salary_id) return;
      if (showHistory.value) { showHistory.value = false; return; }
      historyLoading.value = true; historyError.value = "";
      try { history.value = await financeApi.getSalaryChangeHistory(selectedEmployee.value.salary_id); showHistory.value = true; }
      catch { historyError.value = "修改记录读取失败，请稍后重试"; showHistory.value = true; }
      finally { historyLoading.value = false; }
    };

    const openEditDrawer = () => {
      if (!selectedEmployee.value) return;
      Object.assign(editForm, selectedEmployee.value, { change_reason: "" });
      detailDrawerOpen.value = false;
      editDrawerOpen.value = true;
    };

    const openCompensationQueue = () => void router.push("/finance/compensation");

    const closeDrawers = () => {
      editDrawerOpen.value = false;
      reviewDrawerOpen.value = false;
      detailDrawerOpen.value = false;
    };

    const loadEmployees = async (targetPage = currentPage.value) => {
      const requestedPage = Math.max(1, targetPage);
      const chunkPage = Math.floor((requestedPage - 1) / 2) + 1;
      let rows = pageChunks.get(chunkPage);
      if (!rows) {
        const result = await financeApi.searchSalaryEmployees({ keyword: keywordInput.value, payType: payTypeFilter.value, modified: modifiedFilter.value, page: chunkPage, pageSize: fetchPageSize });
        rows = result.employees;
        pageChunks.set(chunkPage, rows);
        totalEmployees.value = result.total;
        period.employeeCount = result.total;
        if (result.period) {
          period.status = result.period.status;
          period.versionNo = result.period.versionNo;
          period.rowVersion = result.period.rowVersion ?? 1;
          period.employeeCount = result.period.employeeCount ?? result.total;
          period.unconfiguredCount = result.period.unconfiguredCount ?? 0;
          period.modifiedCount = result.period.modifiedCount ?? 0;
          period.reviewedCount = result.period.reviewedCount ?? 0;
          period.returnedCount = result.period.returnedCount ?? 0;
          period.submittedBy = result.period.submittedBy ?? 0;
          period.submittedAt = result.period.submittedAt ?? "";
          period.supervisorReviewedBy = result.period.supervisorReviewedBy ?? 0;
          period.supervisorReviewedAt = result.period.supervisorReviewedAt ?? "";
          period.supervisorDecision = result.period.supervisorDecision ?? "";
          period.supervisorNote = result.period.supervisorNote ?? "";
          reviewNote.value = result.period.reviewNote ?? "";
          if (result.period.supervisorNote) supervisorNote.value = result.period.supervisorNote;
        }
      }
      employees.value = rows;
      currentPage.value = Math.min(requestedPage, totalPages.value);
    };
    const refreshEmployees = async () => { pageChunks.clear(); selectedEmployee.value = null; currentPage.value = 1; await loadEmployees(1); };
    const refreshAfterConflict = async (error: unknown) => {
      const message = getHttpErrorMessage(error, "操作失败");
      const status = error instanceof AppHttpError ? error.status : undefined;
      if (status === 409) {
        actionError.value = "数据已被其他人处理，已刷新最新状态";
        await refreshEmployees();
        return;
      }
      actionError.value = message;
    };
    const goToPage = (page: number) => { if (page >= 1 && page <= totalPages.value) void loadEmployees(page); };
    const savePayrollRow = async () => { if (!selectedEmployee.value || !editForm.change_reason) return; saving.value = true; actionError.value = ""; try { await financeApi.updatePayrollRow(selectedEmployee.value.id, editForm); await refreshEmployees(); closeDrawers(); } catch (error) { await refreshAfterConflict(error); } finally { saving.value = false; } };
    const reviewSelected = async () => { if (!selectedEmployee.value) return; saving.value = true; actionError.value = ""; try { await financeApi.reviewPayrollEmployee(selectedEmployee.value.id); await refreshEmployees(); } catch (error) { await refreshAfterConflict(error); } finally { saving.value = false; } };
    const reviewSelectedAndClose = async () => { if (!selectedEmployee.value) return; saving.value = true; actionError.value = ""; try { await financeApi.reviewPayrollEmployee(selectedEmployee.value.id); await refreshEmployees(); detailDrawerOpen.value = false; } catch (error) { await refreshAfterConflict(error); } finally { saving.value = false; } };
    const submitReview = async () => {
      saving.value = true;
      actionError.value = "";
      try {
        await financeApi.submitPayrollReview({ reviewNote: reviewNote.value, expectedRowVersion: period.rowVersion });
        await refreshEmployees();
      } catch (error) {
        await refreshAfterConflict(error);
      } finally {
        saving.value = false;
      }
    };
    const approvePeriod = async () => {
      saving.value = true;
      actionError.value = "";
      try {
        await financeApi.supervisorReviewPayroll({
          decision: "approve",
          note: supervisorNote.value,
          expectedRowVersion: period.rowVersion,
        });
        await refreshEmployees();
      } catch (error) {
        await refreshAfterConflict(error);
      } finally {
        saving.value = false;
      }
    };
    const returnPeriod = async () => {
      if (!supervisorNote.value.trim()) {
        actionError.value = "退回必须填写复审意见";
        return;
      }
      saving.value = true;
      actionError.value = "";
      try {
        await financeApi.supervisorReviewPayroll({
          decision: "return",
          note: supervisorNote.value.trim(),
          returnAll: returnAll.value,
          expectedRowVersion: period.rowVersion,
        });
        await refreshEmployees();
      } catch (error) {
        await refreshAfterConflict(error);
      } finally {
        saving.value = false;
      }
    };
    const lockPeriod = async () => {
      saving.value = true;
      actionError.value = "";
      try {
        await financeApi.lockPayroll(period.rowVersion);
        await refreshEmployees();
      } catch (error) {
        await refreshAfterConflict(error);
      } finally {
        saving.value = false;
      }
    };
    const createRevision = async () => {
      saving.value = true;
      actionError.value = "";
      try {
        await financeApi.createPayrollRevision();
        await refreshEmployees();
      } catch (error) {
        await refreshAfterConflict(error);
      } finally {
        saving.value = false;
      }
    };
    const printPreview = () => window.print();
    const printEmployee = (_row?: SalaryEmployeeRow) => window.print();
    onMounted(() => { if (canReadSalary.value) { void store.dispatch("finance/ensureHomeData"); void refreshEmployees(); } });
    return {
      employees, visibleEmployees, selectedEmployee, keywordInput, payTypeFilter, modifiedFilter, period, reviewNote,
      supervisorNote, returnAll, actionError, periodLabel, periodStatusLabel, statusTone, statusHint, workflowSteps, summaryMetrics,
      canSubmitReview, canSubmitReviewPermission, canSupervisorReview, canOpenReviewCenter, canEditSnapshot,
      canReadSalary, canWriteSalary, canActivateSalaryProfile, canReviewSalary, canLockSalary, canCreateRevision,
      editDrawerOpen, reviewDrawerOpen, detailDrawerOpen, summaryComfortable,
      showHistory, history, historyLoading, historyError, saving, editForm,
      money, brief, additions, deductions, standardLabel, reviewStatusLabel, formula,
      selectEmployee, toggleHistory, reviewSelected, reviewSelectedAndClose, openEditDrawer, openCompensationQueue, closeDrawers,
      loadEmployees, refreshEmployees, goToPage, currentPage, totalPages, totalEmployees, uiPageSize,
      savePayrollRow, submitReview, approvePeriod, returnPeriod, lockPeriod, createRevision, printPreview, printEmployee,
    };
  },
});
</script>

<style scoped>
.payroll-page{display:grid;gap:16px;color:#173f69;position:relative;background:linear-gradient(135deg, #f0f6ff 0%, #e0ecff 50%, #f5f8ff 100%) !important;z-index:1}.payroll-page::before{content:"";position:absolute;top:10%;left:20%;width:450px;height:450px;border-radius:50%;background:rgba(47, 125, 203, 0.15);filter:blur(90px);z-index:-1}.payroll-page::after{content:"";position:absolute;bottom:20%;right:15%;width:450px;height:450px;border-radius:50%;background:rgba(6, 182, 212, 0.12);filter:blur(90px);z-index:-1}.panel{border:1px solid rgba(255,255,255,0.5);border-radius:18px;background:rgba(255,255,255,0.45);backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);box-shadow:0 8px 32px 0 rgba(39, 75, 111, 0.06)}.panel-dark{display:flex;align-items:center;justify-content:space-between;gap:18px;padding:22px 26px;border-radius:18px;background:rgba(255, 255, 255, 0.55) !important;backdrop-filter:blur(20px) !important;border:1px solid rgba(255, 255, 255, 0.6) !important;color:#173f69 !important;box-shadow:0 8px 32px 0 rgba(39, 75, 111, 0.05) !important}.kicker{font-size:10px;font-weight:800;letter-spacing:.18em;color:#536a81 !important}.panel-dark h2{margin:7px 0 4px;font-size:24px;color:#173f69 !important}.panel-dark h2 span{font-size:13px;font-weight:500;color:#536a81 !important}.panel-dark p{margin:0;font-size:12px;color:#536a81 !important}.header-actions,.toolbar{display:flex;align-items:center;gap:8px}.dark-button,.light-button,.muted-button,.outline-button,.primary-button,.muted-wide{border:0;border-radius:9px;padding:10px 14px;font-weight:700;cursor:pointer;transition:all 0.2s}.dark-button{background:rgba(255, 255, 255, 0.4) !important;color:#173f69 !important;border:1px solid rgba(255, 255, 255, 0.5) !important}.dark-button:hover{background:rgba(255, 255, 255, 0.6) !important;border-color:#2f7dcb !important}.light-button{background:#2f7dcb !important;color:#fff !important;box-shadow:0 4px 14px rgba(47, 125, 203, 0.25) !important}.light-button:hover{background:#1d4f84 !important}.muted-button,.muted-wide{background:#607892;color:#fff}.muted-button:disabled,.muted-wide:disabled{opacity:.55;cursor:not-allowed}.progress-track{display:flex;justify-content:space-between;padding:16px 24px;border:1px solid rgba(255,255,255,0.5);border-radius:16px;background:rgba(255,255,255,0.45);backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);box-shadow:0 8px 32px 0 rgba(39, 75, 111, 0.06)}.progress-step{display:grid;justify-items:center;gap:7px;min-width:130px;color:#8193a5;font-size:11px}.progress-dot{display:grid;place-items:center;width:28px;height:28px;border-radius:50%;border:1px solid rgba(255, 255, 255, 0.6);background:rgba(255,255,255,0.3)}.progress-step.done{color:#15805d}.progress-step.done .progress-dot{border-color:#15805d;background:#15805d;color:#fff}.progress-step.active{color:#2f7dcb}.progress-step.active .progress-dot{border-color:#2f7dcb;background:rgba(47, 125, 203, 0.15);color:#2f7dcb}.summary-grid{display:grid;grid-template-columns:repeat(6,1fr);gap:10px}.summary-card{display:grid;gap:8px;padding:15px;border:1px solid rgba(255, 255, 255, 0.6) !important;border-radius:14px;background:rgba(255, 255, 255, 0.5) !important;backdrop-filter:blur(10px) !important;box-shadow:0 8px 24px rgba(39, 75, 111, 0.04) !important;transition:all 0.3s ease}.summary-card:hover{transform:translateY(-2px);box-shadow:0 12px 30px rgba(39, 75, 111, 0.08);border-color:rgba(255, 255, 255, 0.8) !important}.summary-card span{font-size:11px;color:#8194a8;font-weight:700}.summary-card strong{font:800 19px 'JetBrains Mono', SFMono-Regular, Menlo, monospace;color:#173f69}.summary-card.positive{background:rgba(242, 250, 246, 0.6) !important;border-color:rgba(211, 238, 223, 0.7) !important}.summary-card.positive strong{color:#15805d}.summary-card.negative{background:rgba(255, 245, 245, 0.6) !important;border-color:rgba(255, 215, 215, 0.7) !important}.summary-card.negative strong{color:#c94b4b}.payroll-layout{display:grid;grid-template-columns:minmax(0,1fr);gap:16px}.ledger-panel{padding:24px;min-width:0;border:1px solid rgba(255,255,255,0.5) !important;border-radius:18px;background:rgba(255,255,255,0.45);backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);box-shadow:0 8px 32px 0 rgba(39, 75, 111, 0.06)}.section-heading,.detail-head{display:flex;justify-content:space-between;align-items:flex-start;gap:16px}.section-heading h3,.detail-head h3,.review-panel h3{margin:0;color:#12243b;font-size:21px}.section-heading p,.detail-head p,.review-panel>p{margin:7px 0 0;color:#8193a5;font-size:11px}.toolbar input,.toolbar select{height:36px;border:1px solid rgba(255, 255, 255, 0.6);border-radius:8px;padding:0 10px;background:rgba(255, 255, 255, 0.5);color:#536a81;transition:border-color 0.2s}.toolbar input:focus,.toolbar select:focus{border-color:#2f7dcb;outline:none}.toolbar input{width:190px}.ledger-scroll{overflow:auto;margin-top:18px;border:1px solid rgba(255, 255, 255, 0.4) !important;border-radius:13px;background:rgba(255, 255, 255, 0.15) !important}.ledger{display:grid;grid-template-columns:1.2fr 1.1fr 0.6fr 0.8fr 0.8fr 0.7fr 1.1fr 1.1fr 1fr 0.8fr;align-items:center;min-width:0;gap:10px}.ledger-head{padding:14px 16px;background:rgba(237, 244, 251, 0.5) !important;color:#536a81 !important;font-size:10px;font-weight:800;border-bottom:1px solid rgba(255, 255, 255, 0.4) !important}.ledger-row{width:100%;min-height:76px;padding:10px 16px;border:0;border-bottom:1px solid rgba(255, 255, 255, 0.3) !important;background:transparent !important;text-align:left;cursor:pointer;color:#173f69 !important;transition:all 0.2s ease}.ledger-row:hover{background:rgba(255, 255, 255, 0.25) !important}.ledger-row.selected{background:rgba(255, 255, 255, 0.5) !important;box-shadow:inset 4px 0 #2f7dcb !important}.ledger-row.blocked{background:rgba(255, 241, 241, 0.4) !important}.employee-cell{display:grid;gap:5px}.employee-cell b{font-size:12px}.employee-cell small{font-size:10px;color:#8496a8}.mono{font:700 11px 'JetBrains Mono', ui-monospace, monospace;white-space:nowrap}.positive{color:#15805d}.negative{color:#c94b4b}.blue{color:#2f7dcb}.total{color:#1d4f84}.modified-mark,.status-mark{display:inline-flex;width:max-content;padding:5px 7px;border-radius:6px;font-style:normal;font-size:9px;font-weight:800}.modified-mark{background:rgba(255, 241, 216, 0.6);color:#b56a0a}.system-mark{font-size:10px;color:#8496a8}.status-mark.pending{background:rgba(255, 244, 221, 0.6);color:#b56a0a}.status-mark.first_reviewed,.status-mark.second_reviewed{background:rgba(232, 247, 239, 0.6);color:#15805d}.status-mark.returned{background:rgba(255, 241, 241, 0.6);color:#c94b4b}.status-mark.blocked{background:rgba(255, 241, 241, 0.6);color:#c94b4b}.status-mark.locked{background:rgba(237, 244, 251, 0.6);color:#2f7dcb}.success-button{background:linear-gradient(135deg,#34a87a,#15805d);color:#fff;border:0;border-radius:9px;padding:10px 14px;font-weight:700;cursor:pointer}.success-button.wide,.danger-button.wide{width:100%}.danger-button{background:linear-gradient(135deg,#e07070,#c94b4b);color:#fff;border:0;border-radius:9px;padding:10px 14px;font-weight:700;cursor:pointer}.action-error{margin:0;font-size:11px;color:#c94b4b}.meta-line{margin:0;font-size:11px;color:#536a81}.checkbox-row{display:flex;align-items:center;gap:8px;font-size:11px;color:#536a81}.row-action{font-size:10px;font-weight:800;color:#2f7dcb}.empty-state{padding:70px;text-align:center;color:#8496a8}.employee-detail{margin-top:18px;padding:18px;border:1px solid #dce7f1;border-radius:13px;background:rgba(247, 250, 253, 0.5)}.outline-button{border:1px solid rgba(255,255,255,0.6);background:rgba(255,255,255,0.3);color:#2f7dcb}.outline-button:hover{background:rgba(255,255,255,0.5)}.detail-cards{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-top:16px}.detail-cards article{display:grid;gap:8px;padding:14px;border:1px solid rgba(255,255,255,0.5) !important;border-radius:10px;background:rgba(255,255,255,0.3) !important;backdrop-filter:blur(5px)}.detail-cards span,.change-history>.label{font-size:10px;font-weight:800;color:#8093a7}.detail-cards b{font-size:11px}.detail-cards strong{font:800 16px 'JetBrains Mono', ui-monospace, monospace;color:#1d4f84}.detail-cards small{font-size:10px;color:#8496a8}.change-card{background:rgba(255, 249, 237, 0.6) !important;border-color:rgba(234, 214, 173, 0.7) !important}.change-card b{color:#b56a0a}.change-history{display:grid;gap:9px;margin-top:16px}.history-row{display:flex;justify-content:space-between;padding:13px;border:1px solid rgba(255,255,255,0.5);border-radius:9px;background:rgba(255,255,255,0.3);text-align:left;color:#2c435d;cursor:pointer;transition:all 0.2s}.history-row:hover{background:rgba(255,255,255,0.5)}.history-row span,.history-empty,.history-expanded{font-size:10px;color:#8496a8}.history-expanded{padding:12px;border-radius:8px;background:rgba(255, 241, 218, 0.7);color:#8a5a14}.primary-button{background:linear-gradient(135deg,#4b95dd,#2f7dcb);color:#fff}.primary-button:hover{transform:translateY(-1px);box-shadow:0 6px 15px rgba(47, 125, 203, 0.25)}.review-panel{display:grid;align-content:start;gap:12px;padding:22px;background:rgba(246, 249, 252, 0.5)}.review-panel>p{margin-top:-7px}.label{font-size:10px;font-weight:800;color:#8093a7}.check-item{position:relative;display:grid;gap:5px;padding:13px;border-radius:10px}.check-item b{font-size:11px}.check-item small{font-size:10px;color:#8496a8}.check-item button{position:absolute;right:13px;top:19px;border:0;background:none;color:#2f7dcb;font-weight:800;cursor:pointer}.check-item.done{background:rgba(234, 247, 240, 0.6)}.check-item.danger{background:rgba(255, 241, 241, 0.6)}.check-item.warning{background:rgba(255, 245, 231, 0.6)}.check-item.neutral{background:rgba(240, 245, 249, 0.6)}.review-panel textarea,.drawer textarea,.drawer input,.drawer select{width:100%;box-sizing:border-box;border:1px solid rgba(255,255,255,0.5);border-radius:9px;padding:11px;background:rgba(255,255,255,0.6);color:#2c435d;font:inherit;font-size:12px}.review-panel textarea{height:92px;resize:none}.lock-note{padding:13px;border-radius:10px;background:rgba(238, 244, 250, 0.6)}.lock-note b{font-size:11px}.lock-note p{margin:8px 0 0;font-size:10px;line-height:1.7;color:#8496a8}.muted-wide{width:100%}.drawer-backdrop{position:fixed;inset:0;z-index:20;display:flex;justify-content:flex-end;background:rgba(17, 39, 62, 0.2)}.drawer{position:relative;width:min(420px,92vw);height:100%;overflow:auto;box-sizing:border-box;padding:30px 26px;border-radius:18px 0 0 18px}.drawer h3{margin:8px 0;color:#12243b}.drawer p{font-size:11px;line-height:1.7;color:#8496a8}.drawer label{display:grid;gap:7px;margin:18px 0;font-size:11px;font-weight:800;color:#536a81}.drawer-close{position:absolute;right:18px;top:15px;border:0;background:none;font-size:26px;color:#8093a7;cursor:pointer}.drawer .primary-button{width:100%;margin-top:12px}@media(max-width:1400px){.summary-grid{grid-template-columns:repeat(3,1fr)}.payroll-layout{grid-template-columns:1fr}.review-panel{grid-template-columns:repeat(2,1fr)}.review-panel h3,.review-panel>p,.review-panel>.label,.review-panel textarea,.review-panel .lock-note{grid-column:1/-1}}@media(max-width:900px){.panel-dark,.section-heading,.detail-head{display:grid}.header-actions,.toolbar{flex-wrap:wrap}.summary-grid{grid-template-columns:repeat(2,1fr)}.detail-cards{grid-template-columns:1fr}.progress-track{overflow:auto;justify-content:flex-start;gap:28px}.review-panel{grid-template-columns:1fr}}
@media print{.panel-dark,.progress-track,.summary-grid,.review-panel,.toolbar,.ledger-foot,.primary-button,.outline-button{display:none!important}.ledger-panel{box-shadow:none;border:0}.ledger-scroll{overflow:visible}.ledger{min-width:0}.employee-detail{break-inside:avoid}}

/* A/C 方案混合：经典深海蓝磨砂玻璃亚克力风 */
.summary-grid{grid-template-columns:repeat(5,minmax(0,1fr));gap:8px}
.summary-card{min-height:64px;padding:9px 13px;border-radius:10px;gap:4px}
.summary-card span{font-size:11px}
.summary-card strong{font-size:16px}
.summary-grid.comfortable{gap:10px}
.summary-grid.comfortable .summary-card{min-height:86px;padding:15px;border-radius:14px;gap:8px}
.summary-grid.comfortable .summary-card span{font-size:11px}
.summary-grid.comfortable .summary-card strong{font-size:19px}
.dark-button{border:1px solid rgba(255,255,255,0.5);background:rgba(255,255,255,0.3);color:#173f69}
.dark-button:hover{border-color:#2f7dcb;color:#2f7dcb}
.light-button{background:#2f7dcb;color:#fff}
.muted-button{background:rgba(255,255,255,0.2);color:#7890a6}
.payroll-layout{grid-template-columns:minmax(0,1fr)}
.review-drawer{width:min(390px,92vw);height:100%;overflow:auto;box-sizing:border-box;padding:30px 24px;border-radius:18px 0 0 18px;background:rgba(255,255,255,0.65);backdrop-filter:blur(25px);border-left:1px solid rgba(255,255,255,0.5)}
.review-drawer .drawer-close{display:block}
.col-r{justify-self:end;text-align:right}
.ledger-head .col-r{justify-self:end}
.group-cell{display:inline-grid;gap:2px;justify-items:end}
.group-cell b{font-size:12px;font-weight:800}
.group-cell .parts{font-size:9px;color:#9aabbd;letter-spacing:.01em;white-space:nowrap}
.ledger-head,.ledger-row{padding-left:10px;padding-right:10px}
.ledger-head{white-space:nowrap}
.ledger-row .mono{font-size:10px}
.ledger-row .row-action{font-size:9px}
.panel-dark{min-height:76px;padding:12px 20px;border-radius:14px}
.panel-dark h2{margin:3px 0 2px;font-size:22px}
.panel-dark p{font-size:11px}
.header-actions{flex-wrap:nowrap;gap:6px}
.header-actions button{padding:8px 10px;font-size:12px;white-space:nowrap}
.payroll-page{display:grid;gap:16px;color:#173f69;height:100vh;overflow:hidden;grid-template-rows:auto auto auto minmax(0,1fr)}
.payroll-layout,.ledger-panel{min-height:0}
.ledger-panel{display:flex;flex-direction:column}
.ledger-scroll{display:flex;flex:1;min-height:0;flex-direction:column;overflow:hidden}
.ledger-head,.ledger-row{flex:0 0 auto}
.empty-state{display:grid;flex:1;place-items:center;padding:24px}
.ledger-foot{display:flex;justify-content:flex-end;padding-top:14px}
.access-denied{padding:34px;text-align:center}.access-denied h3{margin:0;color:#173f69}.access-denied p{margin:10px 0 0;color:#8193a5;font-size:12px}
@media(max-width:1400px){.summary-grid{grid-template-columns:repeat(5,minmax(0,1fr))}.payroll-layout{grid-template-columns:minmax(0,1fr)}}
@media(max-width:900px){.payroll-page{height:auto;overflow:visible}.summary-grid,.summary-grid.comfortable{grid-template-columns:repeat(2,minmax(0,1fr))}.review-drawer{width:min(420px,92vw)}.ledger-scroll{overflow:auto}.ledger{min-width:920px}.ledger-foot{justify-content:center}}

/* 新增工资明细侧拉抽屉 C 方案磨砂亚克力样式 */
.detail-drawer{width:min(440px,92vw);height:100%;padding:30px 24px;border-radius:18px 0 0 18px;display:flex;flex-direction:column;gap:16px;background:rgba(255, 255, 255, 0.65) !important;backdrop-filter:blur(25px) !important;-webkit-backdrop-filter:blur(25px) !important;border-left:1px solid rgba(255, 255, 255, 0.5) !important;box-shadow:-15px 0 40px rgba(17, 39, 62, 0.08) !important;overflow-y:auto;box-sizing:border-box;z-index:21}
.detail-cards{display:grid;grid-template-columns:1fr;gap:10px}
.detail-cards article{display:grid;gap:6px;padding:14px;border:1px solid rgba(255,255,255,0.5) !important;border-radius:10px;background:rgba(255,255,255,0.3) !important;backdrop-filter:blur(5px)}
.detail-cards strong.total-salary{font:800 18px 'JetBrains Mono', ui-monospace, monospace;color:#2f7dcb;margin-top:4px;display:block}
.drawer-actions{display:flex;gap:10px;margin-top:auto}
.success-button{border:0;border-radius:9px;padding:10px 14px;font-weight:700;cursor:pointer;background:#15805d;color:#fff;flex:1}

/* Full-window payroll workspace: the page owns the viewport, the ledger owns the remaining height. */
.payroll-page {
  height: 100%;
  width: 100%;
  min-height: 0;
  grid-template-rows: auto auto auto minmax(0, 1fr);
  gap: 12px;
  padding: 8px 0 0;
  box-sizing: border-box;
  overflow: hidden;
  background: #f7f9fc !important;
}

.payroll-page::before,
.payroll-page::after {
  display: none;
}

.panel,
.panel-dark,
.progress-track,
.ledger-panel {
  border-color: #dbe6f2 !important;
  background: #fff !important;
  box-shadow: 0 1px 3px rgba(26, 55, 87, 0.04) !important;
  backdrop-filter: none !important;
}

.panel-dark {
  min-height: 68px;
  padding: 8px 0 2px;
  border: 0 !important;
  box-shadow: none !important;
  background: transparent !important;
}

.panel-dark h2 {
  color: #101820 !important;
  font-size: 26px;
  letter-spacing: -0.03em;
}

.panel-dark h2 span,
.panel-dark p,
.kicker {
  color: #71839a !important;
}

.header-actions button {
  height: 40px;
  padding: 0 16px;
  border: 1px solid #dbe6f2 !important;
  border-radius: 10px;
  background: #fff !important;
  color: #243b55 !important;
  box-shadow: 0 1px 2px rgba(26, 55, 87, 0.04);
}

.header-actions .light-button {
  background: #2f82d3 !important;
  border-color: #2f82d3 !important;
  color: #fff !important;
}

.header-actions .muted-button:disabled {
  color: #a5b4c4 !important;
  background: #fff !important;
}

.header-actions button:disabled {
  cursor: not-allowed;
  opacity: 0.55;
}

.progress-track {
  position: relative;
  min-height: 72px;
  padding: 10px 34px;
  border-radius: 16px;
  align-items: center;
}

.progress-track::before {
  content: "";
  position: absolute;
  top: 36px;
  left: 74px;
  right: 74px;
  height: 3px;
  background: #dce8f4;
  z-index: 0;
}

.progress-step {
  position: relative;
  z-index: 1;
  min-width: 0;
  flex: 1;
  gap: 5px;
  color: #74869b;
  font-size: 13px;
  font-weight: 700;
}

.progress-dot {
  width: 30px;
  height: 30px;
  border: 3px solid #dce8f4;
  background: #fff;
  font-size: 14px;
}

.progress-step.done .progress-dot {
  border-color: #198a69;
  background: #198a69;
}

.progress-step.active .progress-dot {
  border-color: #2f82d3;
  background: #fff;
  color: #2f82d3;
}

.summary-grid {
  gap: 14px;
}

.summary-card {
  position: relative;
  min-height: 78px;
  padding: 15px 20px;
  border: 1px solid #dbe6f2 !important;
  border-radius: 14px;
  background: #fff !important;
  box-shadow: 0 1px 3px rgba(26, 55, 87, 0.03) !important;
}

.summary-card::before {
  content: "";
  position: absolute;
  top: 20px;
  left: 20px;
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: #9aabc0;
}

.summary-card.positive::before { background: #198a69; }
.summary-card.negative::before { background: #d95353; }
.summary-card span { padding-left: 14px; color: #71839a; }
.summary-card strong { padding-left: 14px; color: #203b58; font-size: 20px; }
.summary-card.positive strong { color: #198a69; }
.summary-card.negative strong { color: #d95353; }

.payroll-layout,
.ledger-panel {
  min-height: 0;
}

.ledger-panel {
  padding: 20px 26px 14px;
  border-radius: 18px;
}

.section-heading {
  align-items: center;
}

.section-heading h3 {
  color: #111820;
  font-size: 23px;
}

.section-heading h3::after {
  content: "（点击行弹出工资明细子界面）";
  margin-left: 14px;
  color: #8fa2b7;
  font-size: 13px;
  font-weight: 500;
}

.toolbar input,
.toolbar select,
.outline-button {
  height: 38px;
  border: 1px solid #dbe6f2 !important;
  border-radius: 9px;
  background: #fff !important;
  color: #526b84;
}

.toolbar input { width: 220px; }

.ledger-scroll {
  margin-top: 16px;
  border-color: #dbe6f2 !important;
  border-radius: 14px;
  background: #fff !important;
}

.ledger-head {
  min-height: 52px;
  padding: 0 16px;
  background: #f7faff !important;
  border-bottom-color: #dbe6f2 !important;
  color: #5d738d !important;
  font-size: 12px;
}

.ledger-row {
  min-height: 84px;
  padding: 10px 16px;
  border-bottom-color: #dbe6f2 !important;
  color: #101820 !important;
}

.ledger-row:hover,
.ledger-row.selected {
  background: #f1f7ff !important;
}

.employee-cell b { font-size: 15px; }
.employee-cell small { color: #7a8ea5; font-size: 12px; }
.ledger-row .mono { font-size: 13px; color: #172b40; }
.ledger-row .positive { color: #101820; }
.ledger-row .negative { color: #101820; }
.ledger-row .total { color: #2f82d3; font-size: 14px; }
.system-mark { padding: 5px 8px; border: 1px solid #dbe6f2; border-radius: 6px; color: #71839a; }
.status-mark { padding: 5px 9px; border: 1px solid #cfeede; background: #ecfaf3 !important; color: #198a69 !important; }
.status-mark.pending { border-color: #f5dfb0; background: #fff7e5 !important; color: #bc7715 !important; }
.status-mark.blocked { border-color: #ffd5d5; background: #fff0f0 !important; color: #d95353 !important; }

.ledger-foot {
  padding-top: 10px;
  background: #fff;
}

@media (max-width: 900px) {
  .saas-content { height: auto; min-height: 100vh; overflow: visible; }
  .payroll-page { height: auto; min-height: 100vh; overflow: visible; }
  .progress-track::before { display: none; }
  .section-heading h3::after { display: none; }
}
</style>
