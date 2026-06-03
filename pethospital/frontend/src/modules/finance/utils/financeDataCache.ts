import type {
  SalaryInformationCache,
  SalaryManagementPayload,
  SalarySummaryCache,
} from "../api/types";

const FINANCE_CACHE_KEYS = {
  salaryManagement: "finance:salary-management:cache",
  salarySummaries: "finance:salary-summaries:cache",
  currentSalaryInformation: "finance:current-salary-information:cache",
};

const readJsonCache = <T>(key: string): T | null => {
  const rawValue = localStorage.getItem(key);

  if (!rawValue) {
    return null;
  }

  try {
    return JSON.parse(rawValue) as T;
  } catch {
    localStorage.removeItem(key);
    return null;
  }
};

const saveJsonCache = <T>(key: string, value: T) => {
  localStorage.setItem(key, JSON.stringify(value));
};

/**
 * 从本地缓存读取财务端工资管理数据。
 * 缓存不存在或格式异常时返回 null。
 */
export const readFinanceSalaryManagementCache = () =>
  readJsonCache<SalaryManagementPayload>(FINANCE_CACHE_KEYS.salaryManagement);

/**
 * 写入财务端工资管理数据本地缓存。
 */
export const saveFinanceSalaryManagementCache = (
  payload: SalaryManagementPayload
) => {
  saveJsonCache(FINANCE_CACHE_KEYS.salaryManagement, payload);
};

/**
 * 从本地缓存读取财务端员工工资摘要分页数据。
 * 该缓存保存当前页的 150 条工资摘要以及分页总数。
 */
export const readFinanceSalarySummariesCache = () =>
  readJsonCache<SalarySummaryCache>(FINANCE_CACHE_KEYS.salarySummaries);

/**
 * 写入财务端员工工资摘要分页数据本地缓存。
 */
export const saveFinanceSalarySummariesCache = (
  payload: SalarySummaryCache
) => {
  saveJsonCache(FINANCE_CACHE_KEYS.salarySummaries, payload);
};

/**
 * 从本地缓存读取当前员工工资详情。
 * 详情缓存只保留一条记录，切换员工时由新的详情覆盖旧数据。
 */
export const readFinanceCurrentSalaryInformationCache = () =>
  readJsonCache<SalaryInformationCache>(
    FINANCE_CACHE_KEYS.currentSalaryInformation
  );

/**
 * 写入当前员工工资详情本地缓存。
 */
export const saveFinanceCurrentSalaryInformationCache = (
  payload: SalaryInformationCache
) => {
  saveJsonCache(FINANCE_CACHE_KEYS.currentSalaryInformation, payload);
};

/**
 * 清空当前员工工资详情缓存。
 */
export const clearFinanceCurrentSalaryInformationCache = () => {
  localStorage.removeItem(FINANCE_CACHE_KEYS.currentSalaryInformation);
};

/**
 * 清空财务端业务缓存。
 */
export const clearFinanceDataCache = () => {
  Object.values(FINANCE_CACHE_KEYS).forEach((key) => {
    localStorage.removeItem(key);
  });
};
