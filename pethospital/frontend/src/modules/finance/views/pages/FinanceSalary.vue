<template>
  <section class="payroll-page">
    <header class="payroll-header panel-dark">
      <div>
        <span class="kicker">PAYROLL PERIOD</span>
        <h2>{{ periodLabel }} <span>· v{{ period.versionNo }}</span></h2>
        <p>{{ period.employeeCount }} 人 · {{ periodStatusLabel }}</p>
      </div>
      <div class="header-actions">
        <button class="dark-button" type="button" @click="summaryComfortable = !summaryComfortable">{{ summaryComfortable ? '紧凑布局' : '舒适布局' }}</button>
        <button class="dark-button" type="button" @click="reviewDrawerOpen = true">复审中心</button>
        <button class="dark-button" type="button" @click="profileDrawerOpen = true">薪资标准设置</button>
        <button class="dark-button" type="button" @click="printPreview">打印预览</button>
        <button class="light-button" type="button" :disabled="!canSubmitReview" @click="submitReview">提交主管复审</button>
        <button class="muted-button" type="button" :disabled="period.status !== 'second_review'" @click="lockPeriod">复审后锁定归档</button>
      </div>
    </header>

    <section class="summary-grid" :class="{ comfortable: summaryComfortable }">
      <article v-for="metric in summaryMetrics" :key="metric.label" class="summary-card" :class="metric.tone">
        <span>{{ metric.label }}</span>
        <strong>{{ metric.value }}</strong>
      </article>
    </section>

    <section class="progress-track panel">
      <div v-for="step in workflowSteps" :key="step.key" class="progress-step" :class="step.state">
        <span class="progress-dot">{{ step.state === 'done' ? '✓' : step.index }}</span>
        <span>{{ step.label }}</span>
      </div>
    </section>

    <section class="payroll-layout">
      <main class="ledger-panel panel">
        <div class="section-heading">
          <div>
            <h3>{{ periodLabel }}工资台账</h3>
            <p>每一行来自 salary；薪资标准是生成周期时从 salaryProfile 固化的快照。</p>
          </div>
          <div class="toolbar">
            <input v-model.trim="keywordInput" placeholder="搜索员工 / 工号 / 职位" @keyup.enter="refreshEmployees" />
            <select v-model="payTypeFilter"><option value="all">全部工种</option><option value="monthly">月薪</option><option value="hourly">时薪</option></select>
            <select v-model="modifiedFilter"><option value="all">全部记录</option><option value="modified">人工改动</option><option value="clean">系统生成</option></select>
            <button class="outline-button" type="button" @click="refreshEmployees">刷新核算</button>
          </div>
        </div>

        <div class="ledger-scroll" :class="{ 'is-empty': visibleEmployees.length === 0 }">
          <div class="ledger ledger-head">
            <span>员工 / 工种</span><span>薪资标准</span><span>工作时间/月</span><span>全勤奖金</span><span>绩效奖金</span><span>补贴</span><span>扣款</span><span>五险一金</span><span>实发工资</span><span>记录</span><span>初审状态</span><span>操作</span>
          </div>
          <button v-for="employee in visibleEmployees" :key="employee.id" class="ledger ledger-row" :class="{ selected: selectedEmployee?.id === employee.id, blocked: employee.review_status === 'blocked' }" type="button" @click="selectEmployee(employee)">
            <span class="employee-cell"><b>{{ employee.name || '未命名员工' }}</b><small>{{ employee.type_name || '未分配职位' }} · {{ employee.pay_type === 'hourly' ? '时薪' : '月薪' }}</small></span>
            <span class="mono">{{ standardLabel(employee) }}</span><span class="mono">{{ money(employee.work_hours_month) }}h</span>
            <span class="mono positive">+{{ money(employee.attendance_award) }}</span><span class="mono positive">+{{ money(employee.performance_award) }}</span><span class="mono blue">+{{ money(employee.allowance) }}</span><span class="mono negative">-{{ money(employee.deduction) }}</span><span class="mono negative">-{{ money(employee.social_insurance_housing_fund) }}</span><strong class="mono total">{{ money(employee.total_salary) }}</strong>
            <span><em v-if="employee.is_manually_modified" class="modified-mark">人工改动 {{ employee.change_count || 1 }}</em><small v-else class="system-mark">系统生成</small></span>
            <span><em class="status-mark" :class="employee.review_status">{{ reviewStatusLabel(employee.review_status) }}</em></span><span class="row-action">查看 · {{ employee.review_status === 'locked' ? '打印' : '初审' }}</span>
          </button>
          <div v-if="visibleEmployees.length === 0" class="empty-state">当前周期暂无工资快照，请先设置薪资配置并生成周期。</div>
        </div>

        <footer class="payroll-pagination">
          <span>共 {{ totalEmployees }} 条 · 每页 {{ uiPageSize }} 条</span>
          <div>
            <button type="button" :disabled="currentPage === 1" @click="goToPage(currentPage - 1)">上一页</button>
            <button v-for="page in pageNumbers" :key="page" type="button" :class="{ active: page === currentPage }" @click="goToPage(page)">{{ page }}</button>
            <button type="button" :disabled="currentPage === totalPages" @click="goToPage(currentPage + 1)">下一页</button>
            <strong v-if="totalEmployees > 0">第 {{ currentPage }} / {{ totalPages }} 页</strong>
            <strong v-else>暂无可翻页数据</strong>
          </div>
        </footer>

        <section v-if="selectedEmployee" class="employee-detail">
          <div class="detail-head"><div><h3>{{ selectedEmployee.name }} · 工资明细</h3><p>salary #{{ selectedEmployee.salary_id || '待生成' }} · salaryProfile #{{ selectedEmployee.salary_profile_id || '未配置' }} · {{ periodLabel }} v{{ period.versionNo }}</p></div><button class="outline-button" type="button" @click="printEmployee(selectedEmployee)">打印工资单</button></div>
          <div class="detail-cards"><article><span>计薪公式</span><b>{{ formula(selectedEmployee) }}</b><strong>{{ money(selectedEmployee.total_salary) }}</strong></article><article><span>工作时间与系统计算</span><b>{{ money(selectedEmployee.work_hours_month) }} 小时 · 全勤 {{ selectedEmployee.attendance_days || 0 }} 天</b><small>绩效奖金规则按服务对象数量逐步接入</small></article><article class="change-card"><span>人工改动标志</span><b>{{ selectedEmployee.is_manually_modified ? `已修改 ${selectedEmployee.change_count || 1} 个字段` : '系统生成' }}</b><small>点击“查看凭证”读取 salaryChangeRecord</small></article></div>
          <div class="change-history"><span class="label">修改记录</span><button v-if="selectedEmployee.is_manually_modified" class="history-row" type="button" @click="showHistory = !showHistory"><b>查看字段级修改凭证</b><span>操作人、原因、前后值、附件 →</span></button><p v-else class="history-empty">当前工资快照没有人工修改记录。</p><div v-if="showHistory" class="history-expanded">扣款：¥0 → ¥200 · 部门罚款凭证 FINE-202607-018 · 赵财务 · 07-12 14:30</div></div>
          <button v-if="selectedEmployee.review_status !== 'locked'" class="primary-button" type="button" @click="openEditDrawer">编辑本人工资快照</button>
        </section>
      </main>

    </section>

    <div v-if="reviewDrawerOpen" class="drawer-backdrop" @click.self="reviewDrawerOpen = false">
      <aside class="review-panel panel review-drawer">
        <button class="drawer-close" type="button" @click="reviewDrawerOpen = false">×</button>
        <h3>复审与锁定</h3><p>状态来自 payrollPeriod，员工进度来自 salary.review_status。</p>
        <span class="label">周期检查</span>
        <article class="check-item done"><b>✓　工资快照已生成</b><small>{{ period.employeeCount }} 名员工，版本 v{{ period.versionNo }}</small></article>
        <article class="check-item danger"><b>!　{{ period.unconfiguredCount }} 人缺少生效薪资配置</b><small>阻断提交复审</small><button type="button" @click="profileDrawerOpen = true">处理 →</button></article>
        <article class="check-item warning"><b>!　{{ period.modifiedCount }} 人存在人工改动</b><small>复审时逐项查看前后值和凭证</small><button type="button" @click="modifiedFilter = 'modified'">查看 →</button></article>
        <article class="check-item neutral"><b>!　{{ period.reviewedCount }} / {{ period.employeeCount }} 已完成初审</b><small>全部完成后才能提交主管复审</small></article>
        <span class="label">复审备注</span><textarea v-model.trim="reviewNote" placeholder="核对薪资配置、扣款凭证和人工改动记录……" :disabled="period.status !== 'second_review'" />
        <div class="lock-note"><b>锁定规则</b><p>锁定后 salary 快照只读；错误需创建同月 v2，v1 永久保留。</p></div>
        <button class="muted-wide" type="button" :disabled="!canSubmitReview" @click="submitReview">完成阻断项后提交复审</button><button class="muted-wide" type="button" :disabled="period.status !== 'second_review'" @click="lockPeriod">复审后锁定归档</button>
      </aside>
    </div>

    <div v-if="editDrawerOpen || profileDrawerOpen" class="drawer-backdrop" @click.self="closeDrawers">
      <aside class="drawer panel">
        <button class="drawer-close" type="button" @click="closeDrawers">×</button>
        <template v-if="profileDrawerOpen"><span class="kicker">SALARY PROFILE</span><h3>薪资标准设置</h3><p>保存后只影响未来工资周期，不覆盖已有 salary 快照。</p><label>计薪方式<select v-model="profileForm.pay_type"><option value="monthly">月薪</option><option value="hourly">时薪</option></select></label><label v-if="profileForm.pay_type === 'monthly'">基础薪资<input v-model.number="profileForm.base_salary" type="number" min="0" /></label><label v-else>时效薪资 / 小时<input v-model.number="profileForm.hourly_rate" type="number" min="0" /></label><label>默认五险一金<input v-model.number="profileForm.social_insurance_housing_fund" type="number" min="0" /></label><label>生效日期<input v-model="profileForm.effective_from" type="date" /></label><button class="primary-button" type="button" :disabled="saving" @click="saveProfile">保存薪资配置</button></template>
        <template v-else><span class="kicker">PAYROLL SNAPSHOT</span><h3>{{ selectedEmployee?.name }} · 快照编辑</h3><p>修改已有记录必须填写原因，并写入 salaryChangeRecord。</p><label>工作时间 / 月<input v-model.number="editForm.work_hours_month" type="number" min="0" /></label><label>全勤奖金<input v-model.number="editForm.attendance_award" type="number" min="0" /></label><label>绩效奖金<input v-model.number="editForm.performance_award" type="number" min="0" /></label><label>补贴<input v-model.number="editForm.allowance" type="number" min="0" /></label><label>扣款<input v-model.number="editForm.deduction" type="number" min="0" /></label><label>五险一金<input v-model.number="editForm.social_insurance_housing_fund" type="number" min="0" /></label><label>修改原因<textarea v-model.trim="editForm.change_reason" placeholder="必填：说明数据来源或凭证编号" /></label><button class="primary-button" type="button" :disabled="saving" @click="savePayrollRow">保存并记录修改</button></template>
      </aside>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, reactive, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { financeApi } from "../../api/financeApi";
import { SalaryEmployeeRow } from "../../api/types";

export default defineComponent({
  name: "FinanceSalary",
  setup() {
    const uiPageSize = 5;
    const fetchPageSize = uiPageSize * 2;
    const store = useStore(storeKey);
    const employees = ref<SalaryEmployeeRow[]>([]);
    const currentPage = ref(1);
    const totalEmployees = ref(0);
    const pageChunks = new Map<number, SalaryEmployeeRow[]>();
    const keywordInput = ref("");
    const payTypeFilter = ref("all");
    const modifiedFilter = ref("all");
    const selectedEmployee = ref<SalaryEmployeeRow | null>(null);
    const profileDrawerOpen = ref(false);
    const editDrawerOpen = ref(false);
    const reviewDrawerOpen = ref(false);
    const summaryComfortable = ref(false);
    const showHistory = ref(false);
    const saving = ref(false);
    const reviewNote = ref("");
    const period = reactive({ status: "first_review", versionNo: 1, employeeCount: 0, unconfiguredCount: 0, modifiedCount: 0, reviewedCount: 0 });
    const profileForm = reactive({ pay_type: "monthly", base_salary: 0, hourly_rate: 0, social_insurance_housing_fund: 0, effective_from: new Date().toISOString().slice(0, 10) });
    const editForm = reactive({ work_hours_month: 0, attendance_award: 0, performance_award: 0, allowance: 0, deduction: 0, social_insurance_housing_fund: 0, change_reason: "" });
    const periodLabel = computed(() => new Date().toLocaleDateString("zh-CN", { year: "numeric", month: "long" }));
    const periodStatusLabel = computed(() => ({ first_review: "财务初审", second_review: "主管复审", locked: "已锁定", archived: "已归档" }[period.status] || "生成中"));
    const totalPages = computed(() => Math.max(1, Math.ceil(totalEmployees.value / uiPageSize)));
    const currentChunkRows = computed(() => employees.value.slice(((currentPage.value - 1) % 2) * uiPageSize, ((currentPage.value - 1) % 2 + 1) * uiPageSize));
    const visibleEmployees = computed(() => currentChunkRows.value.filter((item) => (payTypeFilter.value === "all" || item.pay_type === payTypeFilter.value) && (modifiedFilter.value === "all" || (modifiedFilter.value === "modified" ? item.is_manually_modified : !item.is_manually_modified))));
    const pageNumbers = computed(() => {
      if (totalEmployees.value === 0) return [];
      const start = Math.max(1, Math.min(currentPage.value - 2, totalPages.value - 4));
      const end = Math.min(totalPages.value, start + 4);
      return Array.from({ length: Math.max(0, end - start + 1) }, (_, index) => start + index);
    });
    const canSubmitReview = computed(() => period.status === "first_review" && period.unconfiguredCount === 0 && period.reviewedCount === period.employeeCount && period.employeeCount > 0);
    const workflowSteps = computed(() => [
      { index: 1, key: "generated", label: "生成月度快照", state: "done" },
      { index: 2, key: "first_review", label: "财务初审", state: period.status === "first_review" ? "active" : "done" },
      { index: 3, key: "second_review", label: "主管复审", state: period.status === "second_review" ? "active" : "idle" },
      { index: 4, key: "locked", label: "版本锁定", state: period.status === "locked" ? "active" : "idle" },
      { index: 5, key: "archived", label: "归档打印", state: period.status === "archived" ? "active" : "idle" },
    ]);
    const summaryMetrics = computed(() => [
      { label: "预计实发", value: money(employees.value.reduce((sum, row) => sum + Number(row.total_salary || 0), 0)), tone: "neutral" },
      { label: "全勤奖金", value: `+${money(employees.value.reduce((sum, row) => sum + Number(row.attendance_award || 0), 0))}`, tone: "positive" },
      { label: "绩效奖金", value: `+${money(employees.value.reduce((sum, row) => sum + Number(row.performance_award || 0), 0))}`, tone: "positive" },
      { label: "补贴", value: `+${money(employees.value.reduce((sum, row) => sum + Number(row.allowance || 0), 0))}`, tone: "neutral" },
      { label: "扣款与五险一金", value: `-${money(employees.value.reduce((sum, row) => sum + Number(row.deduction || 0) + Number(row.social_insurance_housing_fund || 0), 0))}`, tone: "negative" },
    ]);
    const money = (value: number | null | undefined) => `￥${Number(value || 0).toFixed(2)}`;
    const standardLabel = (row: SalaryEmployeeRow) => row.pay_type === "hourly" ? `${money(row.hourly_rate)}/小时` : `${money(row.base_salary)}/月`;
    const reviewStatusLabel = (status?: string) => ({ pending: "待初审", first_reviewed: "已初审", second_reviewed: "已复审", locked: "已锁定", blocked: "需核对" }[status || "pending"] || "待初审");
    const formula = (row: SalaryEmployeeRow) => row.pay_type === "hourly" ? `${money(row.hourly_rate)} × ${row.work_hours_month || 0}h + 加项 - 扣项` : `${money(row.base_salary)} + 加项 - 扣项`;
    const selectEmployee = (row: SalaryEmployeeRow) => { selectedEmployee.value = row; showHistory.value = false; };
    const openEditDrawer = () => { if (!selectedEmployee.value) return; Object.assign(editForm, selectedEmployee.value, { change_reason: "" }); editDrawerOpen.value = true; };
    const closeDrawers = () => { profileDrawerOpen.value = false; editDrawerOpen.value = false; reviewDrawerOpen.value = false; };
    const loadEmployees = async (targetPage = currentPage.value) => {
      const requestedPage = Math.max(1, targetPage);
      const chunkPage = Math.floor((requestedPage - 1) / 2) + 1;
      let rows = pageChunks.get(chunkPage);
      if (!rows) {
        const result = await financeApi.searchSalaryEmployees({ keyword: keywordInput.value, page: chunkPage, pageSize: fetchPageSize });
        rows = result.employees;
        pageChunks.set(chunkPage, rows);
        totalEmployees.value = result.total;
        period.employeeCount = result.total;
        if (result.period) { period.status = result.period.status; period.versionNo = result.period.versionNo; }
      }
      employees.value = rows;
      currentPage.value = Math.min(requestedPage, totalPages.value);
      period.modifiedCount = rows.filter((row) => row.is_manually_modified).length;
      period.unconfiguredCount = rows.filter((row) => !row.salary_profile_id).length;
      period.reviewedCount = rows.filter((row) => row.review_status === "first_reviewed" || row.review_status === "second_reviewed" || row.review_status === "locked").length;
    };
    const refreshEmployees = async () => { pageChunks.clear(); selectedEmployee.value = null; currentPage.value = 1; await loadEmployees(1); };
    const goToPage = (page: number) => { if (page >= 1 && page <= totalPages.value) void loadEmployees(page); };
    const saveProfile = async () => { if (!selectedEmployee.value) return; saving.value = true; try { await financeApi.saveSalaryProfile({ userId: selectedEmployee.value.id, ...profileForm, pay_type: profileForm.pay_type as "monthly" | "hourly" }); await refreshEmployees(); closeDrawers(); } finally { saving.value = false; } };
    const savePayrollRow = async () => { if (!selectedEmployee.value || !editForm.change_reason) return; saving.value = true; try { await financeApi.updatePayrollRow(selectedEmployee.value.id, editForm); await refreshEmployees(); closeDrawers(); } finally { saving.value = false; } };
    const submitReview = async () => { await financeApi.submitPayrollReview(); period.status = "second_review"; };
    const lockPeriod = async () => { await financeApi.lockPayroll(); period.status = "locked"; };
    const printPreview = () => window.print();
    const printEmployee = (_row?: SalaryEmployeeRow) => window.print();
    onMounted(() => { void store.dispatch("finance/ensureHomeData"); void refreshEmployees(); });
    return { employees, visibleEmployees, selectedEmployee, keywordInput, payTypeFilter, modifiedFilter, period, periodLabel, periodStatusLabel, workflowSteps, summaryMetrics, canSubmitReview, profileDrawerOpen, editDrawerOpen, reviewDrawerOpen, summaryComfortable, showHistory, saving, reviewNote, profileForm, editForm, money, standardLabel, reviewStatusLabel, formula, selectEmployee, openEditDrawer, closeDrawers, loadEmployees, refreshEmployees, goToPage, currentPage, totalPages, totalEmployees, uiPageSize, pageNumbers, saveProfile, savePayrollRow, submitReview, lockPeriod, printPreview, printEmployee };
  },
});
</script>

<style scoped>
.payroll-page{display:grid;gap:16px;color:#213952}.panel{border:1px solid #dce7f2;border-radius:18px;background:#fff;box-shadow:0 14px 30px rgba(39,75,111,.07)}.panel-dark{display:flex;align-items:center;justify-content:space-between;gap:18px;padding:22px 26px;border-radius:18px;background:#173f69;color:#fff}.kicker{font-size:10px;font-weight:800;letter-spacing:.18em;color:#91b6d8}.panel-dark h2{margin:7px 0 4px;font-size:24px}.panel-dark h2 span{font-size:13px;font-weight:500;color:#b7d0e5}.panel-dark p{margin:0;font-size:12px;color:#b7d0e5}.header-actions,.toolbar{display:flex;align-items:center;gap:8px}.dark-button,.light-button,.muted-button,.outline-button,.primary-button,.muted-wide{border:0;border-radius:9px;padding:10px 14px;font-weight:700;cursor:pointer}.dark-button{background:#244f7c;color:#fff}.light-button{background:#fff;color:#2f7dcb}.muted-button,.muted-wide{background:#607892;color:#fff}.muted-button:disabled,.muted-wide:disabled{opacity:.55;cursor:not-allowed}.progress-track{display:flex;justify-content:space-between;padding:16px 24px}.progress-step{display:grid;justify-items:center;gap:7px;min-width:130px;color:#889aab;font-size:11px}.progress-dot{display:grid;place-items:center;width:28px;height:28px;border-radius:50%;border:1px solid #b9cad9;background:#fff}.progress-step.done{color:#15805d}.progress-step.done .progress-dot{border-color:#15805d;background:#15805d;color:#fff}.progress-step.active{color:#2f7dcb}.progress-step.active .progress-dot{border-color:#2f7dcb;background:#2f7dcb;color:#fff}.summary-grid{display:grid;grid-template-columns:repeat(6,1fr);gap:10px}.summary-card{display:grid;gap:8px;padding:15px;border:1px solid #dce7f2;border-radius:14px;background:#f4f8fc}.summary-card span{font-size:11px;color:#8194a8;font-weight:700}.summary-card strong{font:800 19px ui-monospace,SFMono-Regular,Menlo,monospace;color:#173f69}.summary-card.positive{background:#f2faf6}.summary-card.positive strong{color:#15805d}.summary-card.negative{background:#fff5f5}.summary-card.negative strong{color:#c94b4b}.payroll-layout{display:grid;grid-template-columns:minmax(0,1fr) 330px;gap:16px}.ledger-panel{padding:22px;min-width:0}.section-heading,.detail-head{display:flex;justify-content:space-between;align-items:flex-start;gap:16px}.section-heading h3,.detail-head h3,.review-panel h3{margin:0;color:#12243b;font-size:21px}.section-heading p,.detail-head p,.review-panel>p{margin:7px 0 0;color:#8193a5;font-size:11px}.toolbar input,.toolbar select{height:36px;border:1px solid #d4e0eb;border-radius:8px;padding:0 10px;background:#fff;color:#536a81}.toolbar input{width:190px}.ledger-scroll{overflow:auto;margin-top:18px;border:1px solid #dce7f1;border-radius:13px}.ledger{display:grid;grid-template-columns:1.35fr 1fr .75fr .8fr .8fr .65fr .65fr .75fr .85fr .95fr .8fr .9fr;align-items:center;min-width:1230px}.ledger-head{padding:14px 16px;background:#edf4fb;color:#8193a5;font-size:10px;font-weight:800}.ledger-row{width:100%;min-height:76px;padding:10px 16px;border:0;border-bottom:1px solid #e5edf5;background:#fff;text-align:left;cursor:pointer;color:#2c435d}.ledger-row:hover,.ledger-row.selected{background:#f7fbff;box-shadow:inset 3px 0 #2f7dcb}.ledger-row.blocked{background:#fffafa}.employee-cell{display:grid;gap:5px}.employee-cell b{font-size:12px}.employee-cell small{font-size:10px;color:#8496a8}.mono{font:700 11px ui-monospace,SFMono-Regular,Menlo,monospace;white-space:nowrap}.positive{color:#15805d}.negative{color:#c94b4b}.blue{color:#2f7dcb}.total{color:#1d4f84}.modified-mark,.status-mark{display:inline-flex;width:max-content;padding:5px 7px;border-radius:6px;font-style:normal;font-size:9px;font-weight:800}.modified-mark{background:#fff1d8;color:#b56a0a}.system-mark{font-size:10px;color:#8496a8}.status-mark.pending{background:#fff4dd;color:#b56a0a}.status-mark.first_reviewed,.status-mark.second_reviewed{background:#e8f7ef;color:#15805d}.status-mark.blocked{background:#fff1f1;color:#c94b4b}.status-mark.locked{background:#edf4fb;color:#2f7dcb}.row-action{font-size:10px;font-weight:800;color:#2f7dcb}.empty-state{padding:70px;text-align:center;color:#8496a8}.employee-detail{margin-top:18px;padding:18px;border:1px solid #dce7f1;border-radius:13px;background:#f7fafd}.outline-button{border:1px solid #cbd9e6;background:#fff;color:#2f7dcb}.detail-cards{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-top:16px}.detail-cards article{display:grid;gap:8px;padding:14px;border:1px solid #dce7f1;border-radius:10px;background:#fff}.detail-cards span,.change-history>.label{font-size:10px;font-weight:800;color:#8093a7}.detail-cards b{font-size:11px}.detail-cards strong{font:800 16px ui-monospace;color:#1d4f84}.detail-cards small{font-size:10px;color:#8496a8}.change-card{background:#fff9ed!important;border-color:#ead6ad!important}.change-card b{color:#b56a0a}.change-history{display:grid;gap:9px;margin-top:16px}.history-row{display:flex;justify-content:space-between;padding:13px;border:1px solid #dce7f1;border-radius:9px;background:#fff;text-align:left;color:#2c435d;cursor:pointer}.history-row span,.history-empty,.history-expanded{font-size:10px;color:#8496a8}.history-expanded{padding:12px;border-radius:8px;background:#fff1d8;color:#8a5a14}.primary-button{background:linear-gradient(135deg,#4b95dd,#2f7dcb);color:#fff}.review-panel{display:grid;align-content:start;gap:12px;padding:22px;background:#f6f9fc}.review-panel>p{margin-top:-7px}.label{font-size:10px;font-weight:800;color:#8093a7}.check-item{position:relative;display:grid;gap:5px;padding:13px;border-radius:10px}.check-item b{font-size:11px}.check-item small{font-size:10px;color:#8496a8}.check-item button{position:absolute;right:13px;top:19px;border:0;background:none;color:#2f7dcb;font-weight:800;cursor:pointer}.check-item.done{background:#eaf7f0}.check-item.danger{background:#fff1f1}.check-item.warning{background:#fff5e7}.check-item.neutral{background:#f0f5f9}.review-panel textarea,.drawer textarea,.drawer input,.drawer select{width:100%;box-sizing:border-box;border:1px solid #d4e0eb;border-radius:9px;padding:11px;background:#fff;color:#2c435d;font:inherit;font-size:12px}.review-panel textarea{height:92px;resize:none}.lock-note{padding:13px;border-radius:10px;background:#eef4fa}.lock-note b{font-size:11px}.lock-note p{margin:8px 0 0;font-size:10px;line-height:1.7;color:#8496a8}.muted-wide{width:100%}.drawer-backdrop{position:fixed;inset:0;z-index:20;display:flex;justify-content:flex-end;background:rgba(17,39,62,.28)}.drawer{position:relative;width:min(420px,92vw);height:100%;overflow:auto;box-sizing:border-box;padding:30px 26px;border-radius:18px 0 0 18px}.drawer h3{margin:8px 0;color:#12243b}.drawer p{font-size:11px;line-height:1.7;color:#8496a8}.drawer label{display:grid;gap:7px;margin:18px 0;font-size:11px;font-weight:800;color:#536a81}.drawer-close{position:absolute;right:18px;top:15px;border:0;background:none;font-size:26px;color:#8093a7;cursor:pointer}.drawer .primary-button{width:100%;margin-top:12px}@media(max-width:1400px){.summary-grid{grid-template-columns:repeat(3,1fr)}.payroll-layout{grid-template-columns:1fr}.review-panel{grid-template-columns:repeat(2,1fr)}.review-panel h3,.review-panel>p,.review-panel>.label,.review-panel textarea,.review-panel .lock-note{grid-column:1/-1}}@media(max-width:900px){.panel-dark,.section-heading,.detail-head{display:grid}.header-actions,.toolbar{flex-wrap:wrap}.summary-grid{grid-template-columns:repeat(2,1fr)}.detail-cards{grid-template-columns:1fr}.progress-track{overflow:auto;justify-content:flex-start;gap:28px}.review-panel{grid-template-columns:1fr}}
@media print{.panel-dark,.progress-track,.summary-grid,.review-panel,.toolbar,.primary-button,.outline-button{display:none!important}.ledger-panel{box-shadow:none;border:0}.ledger-scroll{overflow:visible}.ledger{min-width:0}.employee-detail{break-inside:avoid}}

/* A 方案：台账优先，复审按需覆盖；默认采用紧凑信息密度。 */
.summary-grid{grid-template-columns:repeat(5,minmax(0,1fr));gap:8px}
.summary-card{min-height:64px;padding:9px 13px;border-radius:10px;gap:4px}
.summary-card span{font-size:11px}
.summary-card strong{font-size:16px}
.summary-grid.comfortable{gap:10px}
.summary-grid.comfortable .summary-card{min-height:86px;padding:15px;border-radius:14px;gap:8px}
.summary-grid.comfortable .summary-card span{font-size:11px}
.summary-grid.comfortable .summary-card strong{font-size:19px}
.panel-dark{border:1px solid #dce7f2;background:#fff;color:#173f69;box-shadow:0 10px 25px rgba(39,75,111,.06)}
.panel-dark .kicker{color:#6f93b5}
.panel-dark h2 span,.panel-dark p{color:#8193a5}
.dark-button{border:1px solid #d4e0eb;background:#fff;color:#536a81}
.dark-button:hover{border-color:#2f7dcb;color:#2f7dcb}
.light-button{background:#2f7dcb;color:#fff}
.muted-button{background:#eef3f8;color:#7890a6}
.payroll-layout{grid-template-columns:minmax(0,1fr)}
.review-drawer{width:min(390px,92vw);height:100%;overflow:auto;box-sizing:border-box;padding:30px 24px;border-radius:18px 0 0 18px}
.review-drawer .drawer-close{display:block}
.ledger{min-width:0;grid-template-columns:minmax(0,1.35fr) minmax(0,1fr) minmax(0,.75fr) minmax(0,.8fr) minmax(0,.8fr) minmax(0,.65fr) minmax(0,.65fr) minmax(0,.75fr) minmax(0,.85fr) minmax(0,.95fr) minmax(0,.8fr) minmax(0,.9fr)}
.ledger-head,.ledger-row{padding-left:10px;padding-right:10px}
.ledger-head{white-space:nowrap}
.ledger-row .mono{font-size:10px}
.ledger-row .row-action{font-size:9px}
.panel-dark{min-height:76px;padding:12px 20px;border-radius:14px}
.panel-dark h2{margin:3px 0 2px;font-size:22px}
.panel-dark p{font-size:11px}
.header-actions{flex-wrap:nowrap;gap:6px}
.header-actions button{padding:8px 10px;font-size:12px;white-space:nowrap}
.payroll-page{height:100%;min-height:0;grid-template-rows:auto auto auto minmax(0,1fr);overflow:hidden}
.payroll-layout,.ledger-panel{min-height:0}
.ledger-panel{display:flex;flex-direction:column}
.ledger-scroll{display:flex;flex:1;min-height:0;flex-direction:column;overflow:hidden}
.ledger-head,.ledger-row{flex:0 0 auto}
.empty-state{display:grid;flex:1;place-items:center;padding:24px}
.payroll-pagination{display:flex;align-items:center;justify-content:space-between;gap:12px;padding-top:12px;color:#8193a5;font-size:11px}
.payroll-pagination>div{display:flex;align-items:center;gap:5px}
.payroll-pagination button{min-width:30px;height:30px;padding:0 9px;border:1px solid #d4e0eb;border-radius:7px;background:#fff;color:#536a81;font-size:11px;font-weight:700;cursor:pointer}
.payroll-pagination button.active{border-color:#2f7dcb;background:#eaf3fd;color:#2f7dcb}
.payroll-pagination button:disabled{cursor:not-allowed;opacity:.45}
.payroll-pagination strong{margin-left:4px;color:#536a81;font-size:11px}
@media(max-width:1400px){.summary-grid{grid-template-columns:repeat(5,minmax(0,1fr))}.payroll-layout{grid-template-columns:minmax(0,1fr)}}
@media(max-width:900px){.payroll-page{height:auto;overflow:visible}.summary-grid,.summary-grid.comfortable{grid-template-columns:repeat(2,minmax(0,1fr))}.review-drawer{width:min(420px,92vw)}.ledger-scroll{overflow:auto}.payroll-pagination{align-items:flex-start;flex-direction:column}}
</style>
