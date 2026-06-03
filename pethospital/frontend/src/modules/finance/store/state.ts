import { createCacheMeta } from "@/app/store/state";
import { SalaryManagementPayload } from "../api/types";
import { FinanceState } from "./types";

const createSalaryManagementPayload = (): SalaryManagementPayload => ({
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

/**
 * 创建财务模块的初始状态。
 */
export const createFinanceState = (): FinanceState => ({
  salaryManagement: createSalaryManagementPayload(),
  salaryManagementMeta: createCacheMeta(),
});
