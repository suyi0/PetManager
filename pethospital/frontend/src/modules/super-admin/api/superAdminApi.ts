import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { unwrapPagedList } from "@/shared/utils/pagedList";
import {
  CreateUserPayload,
  HomePageSummary,
  UserRow,
  WorkTimeRecord,
  UserLogs,
  SystemLogs,
  AuditLogItem,
  OnlineDoctorsSearchResult,
  UserSearchResult,
  UserRoleCounts,
  LogSearchResult,
} from "./types";

export const superAdminApi = {
  /**
   * 刷新管理员会话
   * @returns 新的访问令牌字符串，或者在请求失败或原令牌无效时抛出错误。
   */
  refreshAdminSession() {
    return http.post("/api/admins/session-renewals");
  },

  /**
   * 获取考勤记录列表。
   * @returns 获取到的考勤记录列表；接口为空时返回空列表。
   */
  async getWorkTimeRecord(): Promise<WorkTimeRecord[]> {
    const { data } = await http.get("/api/admins/work-time-records");
    return unwrapList<WorkTimeRecord>(data);
  },

  async changeDoctorWorkTime(params: {
    user_id: number;
    date: string;
    identifier: "check_in_time" | "check_out_time";
  }): Promise<void> {
    await http.post("/api/admins/doctor-work-time-changes", params);
  },

  async changeDoctorWorkStatus(params: {
    doctorId: number;
    status: "online" | "offline";
  }): Promise<void> {
    await http.post("/api/admins/doctor-work-status-changes", params);
  },

  /**
   * 获取用户列表。
   * @returns 获取到的用户列表；接口为空时返回空列表。
   */
  async getUsers(): Promise<UserRow[]> {
    const { data } = await http.get("/api/admins/users");
    return unwrapList<UserRow>(data);
  },

  async searchUsers(params: {
    keyword: string;
    role?: string;
    page: number;
    pageSize: number;
    includeCounts?: boolean;
  }): Promise<UserSearchResult> {
    const { data } = await http.post("/api/admins/users/search", params);
    const paged = unwrapPagedList<UserRow>(data, params);
    const source = (data as { data?: unknown })?.data ?? data;
    const rc = (source as { roleCounts?: Partial<UserRoleCounts> })?.roleCounts;

    return {
      ...paged,
      roleCounts: rc
        ? {
            all: Number(rc.all ?? paged.total),
            normal: Number(rc.normal ?? 0),
            medical: Number(rc.medical ?? 0),
            admin: Number(rc.admin ?? 0),
          }
        : undefined,
    };
  },

  async searchOnlineDoctors(params: {
    keyword: string;
    page: number;
    pageSize: number;
  }): Promise<OnlineDoctorsSearchResult> {
    const { data } = await http.post(
      "/api/admins/online-doctors/search",
      params
    );
    const payload = data?.data ?? data;
    const paged = unwrapPagedList<UserRow>(data, params);

    return {
      ...paged,
      records: unwrapList<WorkTimeRecord>(payload?.records),
    };
  },

  async createUser(payload: CreateUserPayload): Promise<void> {
    await http.post("/api/admins/users", payload);
  },

  async deleteUser(userID: number): Promise<void> {
    await http.post("/api/admins/user-deletions", { user_id: userID });
  },

  /**
   * 获取用户日志和系统日志。
   * @returns 包含用户日志和系统日志的对象；接口为空时返回空列表。
   */
  async getLogs(): Promise<{ userLogs: UserLogs[]; systemLogs: SystemLogs[] }> {
    const { data } = await http.get("/api/admins/logs");
    const logsPayload = data?.data ?? data;

    return {
      userLogs: unwrapList<UserLogs>(logsPayload?.userLogs),
      systemLogs: unwrapList<SystemLogs>(logsPayload?.systemLogs),
    };
  },

  async searchLogs(params: {
    majorTab: "user" | "system";
    role: string;
    keyword: string;
    module?: string;
    result?: string;
    startDate?: string;
    endDate?: string;
    page: number;
    pageSize: number;
    includeCounts?: boolean;
  }): Promise<LogSearchResult> {
    const { data } = await http.post("/api/admins/logs/search", params);
    const paged = unwrapPagedList<AuditLogItem>(data, params);
    const source = (data as { data?: unknown })?.data ?? data;
    const counts = source as {
      userLogCount?: unknown;
      systemLogCount?: unknown;
    };

    return {
      ...paged,
      userLogCount:
        counts?.userLogCount === undefined
          ? undefined
          : Number(counts.userLogCount),
      systemLogCount:
        counts?.systemLogCount === undefined
          ? undefined
          : Number(counts.systemLogCount),
    };
  },

  /**
   * 获取首页摘要数据，包括用户总数、在线医生总数和日志总数。
   * @returns 包含首页摘要数据的对象；接口失败时抛出错误。
   */
  async homePageGetData(): Promise<HomePageSummary> {
    const { data } = await http.get("/api/admins/home-data");
    const summary = data?.data ?? data;

    return {
      dailyExpense: Number(summary?.dailyExpense ?? 0),
      dailyCost: Number(summary?.dailyCost ?? 0),
      dailySales: Number(summary?.dailySales ?? 0),
      dailyProfit: Number(summary?.dailyProfit ?? 0),
      userCount: Number(summary?.userCount ?? 0),
      onlineDoctorCount: Number(summary?.onlineDoctorCount ?? 0),
      allLogCount: Number(summary?.allLogCount ?? 0),
      userLogCount: Number(summary?.userLogCount ?? 0),
      systemLogCount: Number(summary?.systemLogCount ?? 0),
    };
  },
};
