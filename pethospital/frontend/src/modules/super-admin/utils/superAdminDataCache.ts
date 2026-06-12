import type { HomePageSummary, UserRow, WorkTimeRecord } from "../api/types";
import type { LogsPayload } from "../store/types";
import { createVersionedLocalCacheAccessors } from "@/shared/utils/versionedLocalCache";


const SUPER_ADMIN_CACHE_OPTIONS = {
  version: 1,
  ttlMs: 1000 * 60 * 60 * 24 * 30,
};

/**
 * 超级管理端业务缓存 key
 */
const SUPER_ADMIN_CACHE_KEYS = {
  users: "super-admin:users:cache",
  workTimeRecords: "super-admin:work-time-records:cache",
  logs: "super-admin:logs:cache",
  homePageData: "super-admin:home-page-data:cache",
};

const superAdminCache = createVersionedLocalCacheAccessors(
  SUPER_ADMIN_CACHE_OPTIONS
);

/**
 * 从本地缓存读取超级管理端用户列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readSuperAdminUsersCache = () =>
  superAdminCache.readArray<UserRow>(SUPER_ADMIN_CACHE_KEYS.users);

/**
 * 写入超级管理端用户列表本地缓存。
 */
export const saveSuperAdminUsersCache = (users: UserRow[]) => {
  superAdminCache.save(SUPER_ADMIN_CACHE_KEYS.users, users);
};

/**
 * 从本地缓存读取超级管理端考勤记录。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readSuperAdminWorkTimeRecordsCache = () =>
  superAdminCache.readArray<WorkTimeRecord>(
    SUPER_ADMIN_CACHE_KEYS.workTimeRecords
  );

/**
 * 写入超级管理端考勤记录本地缓存。
 */
export const saveSuperAdminWorkTimeRecordsCache = (
  records: WorkTimeRecord[]
) => {
  superAdminCache.save(SUPER_ADMIN_CACHE_KEYS.workTimeRecords, records);
};

/**
 * 从本地缓存读取超级管理端日志数据。
 * 缓存不存在或格式异常时返回 null。
 */
export const readSuperAdminLogsCache = () =>
  superAdminCache.read<LogsPayload>(SUPER_ADMIN_CACHE_KEYS.logs);

/**
 * 写入超级管理端日志数据本地缓存。
 */
export const saveSuperAdminLogsCache = (logs: LogsPayload) => {
  superAdminCache.save(SUPER_ADMIN_CACHE_KEYS.logs, logs);
};

/**
 * 从本地缓存读取超级管理端首页摘要数据。
 * 缓存不存在或格式异常时返回 null。
 */
export const readSuperAdminHomePageDataCache = () =>
  superAdminCache.read<HomePageSummary>(SUPER_ADMIN_CACHE_KEYS.homePageData);

/**
 * 写入超级管理端首页摘要数据本地缓存。
 */
export const saveSuperAdminHomePageDataCache = (summary: HomePageSummary) => {
  superAdminCache.save(SUPER_ADMIN_CACHE_KEYS.homePageData, summary);
};

/**
 * 清空超级管理端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧管理数据。
 */
export const clearSuperAdminDataCache = () => {
  superAdminCache.clearAll(Object.values(SUPER_ADMIN_CACHE_KEYS));
};
