import http from "@/api/http";
import { unwrapList } from "@/api/response";
import {
  ChangeSalaryPayload,
  FinanceHomeData,
  SalaryInformationCache,
  SalaryEmployeeRow,
  SalaryManagementPayload,
  SalarySummaryCache,
  SalarySummaryItem,
  PayrollRowPayload,
  SalaryProfilePayload,
  PayrollPeriodSummary,
  PayrollAuditEvent,
  SalaryChangeRecord,
} from "./types";

interface SalaryEmployeeSearchResult {
  employees: SalaryEmployeeRow[];
  total: number;
  page: number;
  pageSize: number;
  summary: SalaryManagementPayload["summary"];
  period?: PayrollPeriodSummary;
}

const createEmptySalaryManagementPayload = (): SalaryManagementPayload => ({
  summary: {
    employeeCount: 0,
    monthlyPayroll: 0,
    todayCost: 0,
    todayProfit: 0,
  },
  employees: [],
  monthlyRecords: [],
  dailyRecords: [],
});

const createEmptySalarySummaryCache = (page = 1): SalarySummaryCache => ({
  list: [],
  total_count: 0,
  page,
  page_size: 150,
});

const createEmptyFinanceHomeData = (): FinanceHomeData => ({
  dailyExpense: 0,
  dailyCost: 0,
  dailySales: 0,
  dailyProfit: 0,
  dailyUnassignedSales: 0,
});

export const financeApi = {
  /**
   * 获取财务端首页总览数据。
   * 接口路径对应后端 /api/finance/home-data。
   */
  async getHomeData(): Promise<FinanceHomeData> {
    try {
      const { data } = await http.get("/api/finance/home-data");
      const payload = data?.data ?? data;

      return {
        dailyExpense: Number(payload?.dailyExpense ?? 0),
        dailyCost: Number(payload?.dailyCost ?? payload?.costCount ?? 0),
        dailySales: Number(payload?.dailySales ?? payload?.salesCount ?? 0),
        dailyProfit: Number(payload?.dailyProfit ?? payload?.profitCount ?? 0),
        dailyUnassignedSales: Number(payload?.dailyUnassignedSales ?? 0),
      };
    } catch {
      return createEmptyFinanceHomeData();
    }
  },

  /**
   * 获取财务端工资管理数据。
   */
  async getSalaryManagementData(): Promise<SalaryManagementPayload> {
    try {
      const { data } = await http.get("/api/finance/expenses");
      const payload = data?.data ?? data;

      return {
        summary: {
          employeeCount: Number(payload?.summary?.employeeCount ?? 0),
          monthlyPayroll: Number(payload?.summary?.monthlyPayroll ?? 0),
          todayCost: Number(payload?.summary?.todayCost ?? 0),
          todayProfit: Number(payload?.summary?.todayProfit ?? 0),
        },
        employees: unwrapList(payload?.employees),
        monthlyRecords: unwrapList(payload?.monthlyRecords),
        dailyRecords: unwrapList(payload?.dailyRecords),
      };
    } catch {
      return createEmptySalaryManagementPayload();
    }
  },

  async searchSalaryEmployees(params: {
    keyword: string;
    payType?: string;
    modified?: string;
    reviewStatus?: string;
    page: number;
    pageSize: number;
  }): Promise<SalaryEmployeeSearchResult> {
    const { data } = await http.post("/api/finance/salary-employees/search", {
      keyword: params.keyword,
      payType: params.payType ?? "all",
      modified: params.modified ?? "all",
      reviewStatus: params.reviewStatus ?? "all",
      page: params.page,
      pageSize: params.pageSize,
    });
    const payload = data?.data ?? data;
    const employees = unwrapList<SalaryEmployeeRow>(payload?.employees);

    return {
      employees,
      total: Number(payload?.total ?? employees.length),
      page: Number(payload?.page ?? params.page),
      pageSize: Number(payload?.pageSize ?? params.pageSize),
      summary: {
        employeeCount: Number(payload?.summary?.employeeCount ?? 0),
        monthlyPayroll: Number(payload?.summary?.monthlyPayroll ?? 0),
        todayCost: Number(payload?.summary?.todayCost ?? 0),
        todayProfit: Number(payload?.summary?.todayProfit ?? 0),
      },
      period: payload?.period
        ? {
            id: Number(payload.period.id ?? payload.period.periodId ?? 0),
            periodId: Number(payload.period.periodId ?? payload.period.id ?? 0),
            status: payload.period.status ?? "first_review",
            versionNo: Number(payload.period.versionNo ?? 1),
            rowVersion: Number(payload.period.rowVersion ?? 1),
            totalSalary: Number(payload.period.totalSalary ?? 0),
            employeeCount: Number(payload.period.employeeCount ?? 0),
            unconfiguredCount: Number(payload.period.unconfiguredCount ?? 0),
            pendingReviewCount: Number(payload.period.pendingReviewCount ?? 0),
            modifiedCount: Number(payload.period.modifiedCount ?? 0),
            reviewedCount: Number(payload.period.reviewedCount ?? 0),
            returnedCount: Number(payload.period.returnedCount ?? 0),
            reviewNote: payload.period.reviewNote ?? "",
            submittedBy: Number(payload.period.submittedBy ?? 0),
            submittedAt: payload.period.submittedAt ?? "",
            supervisorReviewedBy: Number(payload.period.supervisorReviewedBy ?? 0),
            supervisorReviewedAt: payload.period.supervisorReviewedAt ?? "",
            supervisorDecision: payload.period.supervisorDecision ?? "",
            supervisorNote: payload.period.supervisorNote ?? "",
          }
        : undefined,
    };
  },

  /**
   * 获取员工工资摘要分页数据。
   * 后端固定每页返回 150 条，前端按页码缓存当前页摘要列表。
   */
  async getSalarySummary(page = 1): Promise<SalarySummaryCache> {
    try {
      const { data } = await http.get(`/api/finance/salary-summaries/${page}`);
      const payload = data?.data ?? data;
      const list = unwrapList<SalarySummaryItem>(payload?.list).map((item) => ({
        salary_id: Number(item.salary_id ?? 0),
        employee_name: item.employee_name ?? "",
        type: item.type ?? "",
        total_salary: Number(item.total_salary ?? 0),
      }));

      return {
        list,
        total_count: Number(payload?.total_count ?? list.length),
        page: Number(payload?.page ?? page),
        page_size: 150,
      };
    } catch {
      return createEmptySalarySummaryCache(page);
    }
  },

  /**
   * 获取员工工资详情。
   * 前端只缓存当前查看的一条详情记录，切换工资单时用新记录覆盖旧缓存。
   */
  async getSalaryInformation(
    salaryId: number
  ): Promise<SalaryInformationCache | null> {
    try {
      const { data } = await http.get(
        `/api/finance/salary-records/${salaryId}`
      );
      const payload = data?.data ?? data;

      return {
        user_id: Number(payload?.user_id ?? 0),
        salary_id: Number(payload?.salary_id ?? salaryId),
        name: payload?.name ?? "",
        type: payload?.type ?? "",
        base_salary: Number(payload?.base_salary ?? 0),
        PA_Award: Number(payload?.PA_Award ?? 0),
        PB_Award: Number(payload?.PB_Award ?? 0),
        total_salary: Number(payload?.total_salary ?? 0),
        updated_at: payload?.updated_at ?? "",
      };
    } catch {
      return null;
    }
  },

  /**
   * 修改员工工资结构。
   */
  async changeSalary(payload: ChangeSalaryPayload): Promise<void> {
    await http.post(`/api/finance/employee-salaries/${payload.userId}`, {
      baseSalary: payload.baseSalary,
      PA_Award: payload.paAward,
      PB_Award: payload.pbAward,
    });
  },

  async saveSalaryProfile(payload: SalaryProfilePayload): Promise<void> {
    await http.post(`/api/finance/employees/${payload.userId}/salary-profile`, payload);
  },

  async getSalaryProfile(userId: number): Promise<SalaryProfilePayload | null> {
    const { data } = await http.get(`/api/finance/employees/${userId}/salary-profile`);
    const payload = data?.data ?? data;
    if (!payload) return null;
    return {
      userId,
      pay_type: payload.pay_type === "hourly" ? "hourly" : "monthly",
      base_salary: payload.base_salary == null ? null : Number(payload.base_salary),
      hourly_rate: payload.hourly_rate == null ? null : Number(payload.hourly_rate),
      social_insurance_housing_fund: Number(payload.social_insurance_housing_fund ?? 0),
      effective_from: payload.effective_from ?? new Date().toISOString().slice(0, 10),
    };
  },

  async updatePayrollRow(userId: number, payload: PayrollRowPayload): Promise<void> {
    await http.post(`/api/finance/employee-salaries/${userId}`, payload);
  },

  async getSalaryChangeHistory(salaryId: number): Promise<SalaryChangeRecord[]> {
    const { data } = await http.get(`/api/finance/salary-records/${salaryId}/change-history`);
    const payload = data?.data ?? data;
    return unwrapList<SalaryChangeRecord>(payload?.items);
  },

  async reviewPayrollEmployee(userId: number): Promise<void> {
    await http.post(`/api/finance/payroll-periods/current/employees/${userId}/first-review`);
  },

  async submitPayrollReview(payload: {
    reviewNote?: string;
    expectedRowVersion?: number;
  } = {}): Promise<void> {
    await http.post("/api/finance/payroll-periods/current/submit-review", {
      reviewNote: payload.reviewNote ?? "",
      expectedRowVersion: payload.expectedRowVersion,
    });
  },

  async supervisorReviewPayroll(payload: {
    decision: "approve" | "return";
    note?: string;
    returnedSalaryIds?: number[];
    returnAll?: boolean;
    expectedRowVersion?: number;
  }): Promise<void> {
    await http.post("/api/finance/payroll-periods/current/supervisor-review", payload);
  },

  async lockPayroll(expectedRowVersion?: number): Promise<void> {
    await http.post("/api/finance/payroll-periods/current/lock", {
      expectedRowVersion,
    });
  },

  async createPayrollRevision(): Promise<void> {
    await http.post("/api/finance/payroll-periods/current/revisions");
  },

  async getPayrollAuditEvents(): Promise<PayrollAuditEvent[]> {
    const { data } = await http.get("/api/finance/payroll-periods/current/audit-events");
    const payload = data?.data ?? data;
    return unwrapList<PayrollAuditEvent>(payload?.items);
  },
};
