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

export type AttendanceStatus =
  | "normal"
  | "late"
  | "early_leave"
  | "late_and_early"
  | "missing_out"
  | "absent";

export interface AttendanceRecord {
  user_id: number;
  name: string;
  department_name: string;
  work_date: string;
  check_in_at: string;
  check_out_at: string;
  status: AttendanceStatus;
  is_corrected: boolean;
  correction_note: string;
}

export interface AttendanceRecordQuery {
  month?: string;
  start_date?: string;
  end_date?: string;
  user_id?: number;
  status?: AttendanceStatus | "";
}

export interface AttendanceRecordResult {
  items: AttendanceRecord[];
  total: number;
}

export interface AttendanceDevice {
  id: number;
  name: string;
  device_key: string;
  vendor: string;
  location: string;
  branch_id: number | null;
  is_active: boolean;
  last_seen_at: string;
  created_at: string;
}

export interface AttendanceDeviceCreateResult {
  id: number;
  device_key: string;
  secret: string;
}

export interface AttendanceSecretResult {
  device_id: number;
  secret: string;
}

export interface UserRow {
  id: number;
  type_id: number | null;
  type_name?: string;
  name: string;
  phone: string;
  email: string;
  birthday: string;
  head_image: string;
  status?: "online" | "offline" | "";
}

export interface RbacDepartment {
  id: number;
  branch_id: number;
  branch_name?: string;
  name: string;
  description?: string;
  business_domain?: string;
  is_system?: boolean;
}

export interface RbacPosition {
  id: number;
  department_id: number | null;
  department_name?: string;
  name: string;
  system_key?: string;
  staff_kind?: string;
  description?: string;
}

export interface PermissionTemplate {
  id: number;
  name: string;
}

export interface UserScopePayload {
  user_id: number;
  branch_ids: number[];
  department_ids: number[];
}

export interface PositionPermissionsPayload {
  permissions: string[];
  grantableKeys: string[];
}

export interface UserRbacPermissions {
  positionId: number;
  positionName: string;
  departmentName: string;
  positionPermissions: string[];
  personalPermissions: string[];
  grantableKeys: string[];
}

export interface SalaryEmployeeRow {
  id: number;
  type_id: number | null;
  type_name?: string;
  name: string;
  phone: string;
  email: string;
  base_salary: number;
  pa_award: number;
  pb_award: number;
  total_salary: number;
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

export interface CreateUserPayload {
  name: string;
  phone?: string;
  email?: string;
  password?: string;
  birthday?: string;
}

export interface ApiListResponse<T> {
  success?: boolean;
  data?: T[];
  message?: string;
}

export interface PagedList<T> {
  items: T[];
  total: number;
  page: number;
  pageSize: number;
}

export interface OnlineDoctorsSearchResult extends PagedList<UserRow> {
  records: WorkTimeRecord[];
}

export interface UserRoleCounts {
  all: number;
  normal: number;
  medical: number;
  admin: number;
}

export interface UserSearchResult extends PagedList<UserRow> {
  // 仅在请求 includeCounts 时返回；翻页/切角色等场景为 undefined，前端复用缓存。
  roleCounts?: UserRoleCounts;
}

export interface LogSearchResult extends PagedList<AuditLogItem> {
  // 仅在请求 includeCounts 时返回；否则为 undefined，前端复用缓存。
  userLogCount?: number;
  systemLogCount?: number;
}

/**
 * 超级管理员首页摘要数据。
 */
export interface HomePageSummary {
  dailyExpense: number;
  dailyCost: number;
  dailySales: number;
  dailyProfit: number;
  userCount: number;
  onlineDoctorCount: number;
  allLogCount: number;
  userLogCount: number;
  systemLogCount: number;
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
  | "总裁"
  | "副总裁"
  | "财务总监"
  | "部门经理"
  | "普通用户"
  | "护士"
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
