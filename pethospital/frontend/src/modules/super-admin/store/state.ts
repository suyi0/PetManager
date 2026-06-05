import { HomePageSummary } from "../api/types";
import { LogsPayload, SuperAdminState } from "./types";
import { createCacheMeta } from "@/app/store/state";

/**
 * 日志模块的默认空数据。
 */
const createLogsPayload = (): LogsPayload => ({
  userLogs: [],
  systemLogs: [],
});

/**
 * 首页摘要的默认空数据。
 */
const createHomePageSummary = (): HomePageSummary => ({
  dailyExpense: 0,
  dailyCost: 0,
  dailySales: 0,
  dailyProfit: 0,
  userCount: 0,
  onlineDoctorCount: 0,
  allLogCount: 0,
  userLogCount: 0,
  systemLogCount: 0,
});

/**
 * 创建超级管理员模块的初始状态。
 */
export const createSuperAdminState = (): SuperAdminState => ({
  users: [],
  workTimeRecords: [],
  logs: createLogsPayload(),
  homePageData: createHomePageSummary(),
  usersMeta: createCacheMeta(),
  workTimeRecordsMeta: createCacheMeta(),
  logsMeta: createCacheMeta(),
  homePageDataMeta: createCacheMeta(),
});
