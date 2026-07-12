export interface FinanceHomeData {
  dailyExpense: number;
  dailyCost: number;
  dailySales: number;
  dailyProfit: number;
}

export interface SalaryEmployeeRow {
  id: number;
  type_id: number | null;
  type_name?: string;
  name: string;
  phone: string;
  email: string;
  salary_id?: number;
  salary_profile_id?: number | null;
  pay_type: "monthly" | "hourly";
  base_salary: number | null;
  hourly_rate: number | null;
  work_hours_month: number;
  attendance_award: number;
  performance_award: number;
  allowance: number;
  deduction: number;
  social_insurance_housing_fund: number;
  total_salary: number;
  review_status?: string;
  is_manually_modified?: boolean;
  change_count?: number;
  attendance_days?: number;
  updated_at: string;
}

export interface SalaryRecordRow {
  id: string;
  salesCount: number;
  costCount: number;
  profitCount: number;
  created_at: string;
}

export interface SalaryManagementSummary {
  employeeCount: number;
  monthlyPayroll: number;
  todayCost: number;
  todayProfit: number;
}

export interface SalaryManagementPayload {
  summary: SalaryManagementSummary;
  employees: SalaryEmployeeRow[];
  monthlyRecords: SalaryRecordRow[];
  dailyRecords: SalaryRecordRow[];
}

export interface SalarySummaryItem {
  salary_id: number;
  employee_name: string;
  type: string;
  total_salary: number;
}

export interface SalarySummaryCache {
  list: SalarySummaryItem[];
  total_count: number;
  page: number;
  page_size: 150;
}

export interface SalaryInformationCache {
  user_id: number;
  salary_id: number;
  name: string;
  type: string;
  base_salary: number;
  PA_Award: number;
  PB_Award: number;
  total_salary: number;
  updated_at: string;
}

export interface ChangeSalaryPayload {
  userId: number;
  baseSalary: number;
  paAward: number;
  pbAward: number;
}

export interface SalaryProfilePayload {
  userId: number;
  pay_type: "monthly" | "hourly";
  base_salary: number | null;
  hourly_rate: number | null;
  social_insurance_housing_fund: number;
  effective_from: string;
}

export interface PayrollRowPayload {
  work_hours_month: number;
  attendance_award: number;
  performance_award: number;
  allowance: number;
  deduction: number;
  social_insurance_housing_fund: number;
  change_reason: string;
}

export interface PayrollPeriodSummary {
  id: number;
  status: string;
  versionNo: number;
  totalSalary: number;
}
