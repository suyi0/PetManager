export interface WorkTimeRecord {
  source: "online_doctors" | "work_records";
  id: number;
  user_id: number;
  name: string;
  date: string;
  check_in_time: string;
  check_out_time: string;
  status: string;
  notes: string;
  created_at: string;
  updated_at: string;
}

export interface UserRow {
  id: number;
  type_id: number | null;
  type_name?: string;
  name: string;
  phone: string;
  email: string;
  birthday: string;
  address_id: string;
  head_image: string;
  status?: "online" | "offline" | "";
}

export interface CreateUserPayload {
  name: string;
  phone?: string;
  email?: string;
  password?: string;
  birthday?: string;
  address_id?: number;
}

export interface ApiListResponse<T> {
  success?: boolean;
  data?: T[];
  message?: string;
}

/**
 * 超级管理员首页摘要数据。
 */
export interface HomePageSummary {
  userCount: number;
  onlineDoctorCount: number;
  logsCount: number;
}
/**
 * 日志大类枚举，包含用户类和系统类两种类型
 */
export type MajorTab = "user" | "system";
/**
 * 用户角色枚举，包含所有用户角色和未指定角色的选项
 */
export type UserRole =
  | "all"
  | "普通用户"
  | "医生"
  | "仓库管理员"
  | "超级管理员";
/**
 * 日志类别枚举，包含用户类和系统类两种类型
 */
export type LogCategory = "用户类" | "系统类";
/**
 * 操作结果枚举，包含成功、警告和失败三种状态
 */
export type LogResult = "成功" | "警告" | "失败";
/**
 * 审计日志项，包含用户类日志和系统类日志两种类型
 * - 用户类日志包含用户角色字段，系统类日志不区分角色
 * - 通过 category 字段区分日志类型，category 为 "用户类" 时为用户类日志，category 为 "系统类" 时为系统类日志
 */
export type AuditLogItem = UserLogs | SystemLogs;

export interface UserLogs {
  id: string; // 日志ID
  category: LogCategory; // 日志类别
  userRole?: Exclude<UserRole, "all">; // 用户角色（仅用户类日志）
  operator: string; // 操作者
  module: string; // 模块
  action: string; // 操作
  result: LogResult; // 操作结果
  time: string; // 操作时间
  summary: string; // 操作摘要
  details: string; // 操作详情
  source: string; // 操作来源
}

export interface SystemLogs {
  id: string; // 日志ID
  category: LogCategory; // 日志类别
  operator: string; // 操作者
  module: string; // 模块
  action: string; // 操作
  result: LogResult; // 操作结果
  time: string; // 操作时间
  summary: string; // 操作摘要
  details: string; // 操作详情
  source: string; // 操作来源
}
