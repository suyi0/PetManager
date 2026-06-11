import type {
  FinanceHomeData,
  SalaryInformationCache,
  SalaryManagementPayload,
  SalarySummaryCache,
} from "../api/types";
import {
  readVersionedLocalCache,
  saveVersionedLocalCache,
} from "@/shared/utils/versionedLocalCache";

const FINANCE_CACHE_OPTIONS = {
  version: 1,
  ttlMs: 1000 * 60 * 60 * 24 * 30,
};

const FINANCE_CACHE_KEYS = {
  homeData: "finance:home-data:cache",
  salaryManagement: "finance:salary-management:cache",
  salarySummaries: "finance:salary-summaries:cache",
  currentSalaryInformation: "finance:current-salary-information:cache",
};

const readJsonCache = <T>(key: string): T | null => {
  return readVersionedLocalCache<T>(key, FINANCE_CACHE_OPTIONS);
};

const saveJsonCache = <T>(key: string, value: T) => {
  saveVersionedLocalCache(key, value, FINANCE_CACHE_OPTIONS);
};

/**
 * 从本地缓存读取财务端首页统计数据。
 * WebSocket 推送成功后会持续覆盖这份缓存。
 */
export const readFinanceHomeDataCache = () =>
  readJsonCache<FinanceHomeData>(FINANCE_CACHE_KEYS.homeData);

/**
 * 写入财务端首页统计数据本地缓存。
 */
export const saveFinanceHomeDataCache = (payload: FinanceHomeData) => {
  saveJsonCache(FINANCE_CACHE_KEYS.homeData, payload);
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
