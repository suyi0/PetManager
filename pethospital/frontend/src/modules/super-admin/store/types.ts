import {
  HomePageSummary,
  SalaryManagementPayload,
  SystemLogs,
  UserLogs,
  UserRow,
  WorkTimeRecord,
} from "../api/types";
import { CacheMeta } from "@/app/store/types";

/**
 * 日志页需要同时缓存用户日志和系统日志，这里统一成一个对象。
 */
export interface LogsPayload {
  userLogs: UserLogs[];
  systemLogs: SystemLogs[];
}

/**
 * 超级管理员端的业务缓存状态。
 * 这部分只存在于内存里，浏览器关闭后会自然丢失。
 */
export interface SuperAdminState {
  users: UserRow[];
  workTimeRecords: WorkTimeRecord[];
  logs: LogsPayload;
  homePageData: HomePageSummary;
  salaryManagement: SalaryManagementPayload;
  usersMeta: CacheMeta;
  workTimeRecordsMeta: CacheMeta;
  logsMeta: CacheMeta;
  homePageDataMeta: CacheMeta;
  salaryManagementMeta: CacheMeta;
}
