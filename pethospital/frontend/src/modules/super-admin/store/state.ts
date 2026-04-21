import { HomePageSummary } from "../api/types";
import { LogsPayload, SuperAdminState } from "./types";
import { createCacheMeta } from "@/store/state";

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
  userCount: 0,
  onlineDoctorCount: 0,
  logsCount: 0,
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
