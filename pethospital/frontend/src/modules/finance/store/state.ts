import { createCacheMeta } from "@/app/store/state";
import { FinanceHomeData, SalaryManagementPayload } from "../api/types";
import { FinanceState } from "./types";

const createFinanceHomeData = (): FinanceHomeData => ({
  dailyExpense: 0,
  dailyCost: 0,
  dailySales: 0,
  dailyProfit: 0,
  dailyUnassignedSales: 0,
});

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
  homeData: createFinanceHomeData(),
  homeDataMeta: createCacheMeta(),
  salaryManagement: createSalaryManagementPayload(),
  salaryManagementMeta: createCacheMeta(),
});
