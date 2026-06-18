import http from "@/api/http";
import { unwrapList } from "@/api/response";
import {
  CreateUserPayload,
  HomePageSummary,
  UserRow,
  WorkTimeRecord,
  UserLogs,
  SystemLogs,
  AuditLogItem,
  OnlineDoctorsSearchResult,
  PagedList,
} from "./types";

const unwrapPagedList = <T>(
  payload: unknown,
  page: number,
  pageSize: number
): PagedList<T> => {
  const data = (payload as { data?: unknown })?.data ?? payload;
  const source = data as {
    items?: unknown;
    total?: unknown;
    page?: unknown;
    pageSize?: unknown;
  };
  const items = unwrapList<T>(source?.items);

  return {
    items,
    total: Number(source?.total ?? items.length),
    page: Number(source?.page ?? page),
    pageSize: Number(source?.pageSize ?? pageSize),
  };
};

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
  }): Promise<PagedList<UserRow>> {
    const { data } = await http.post("/api/admins/users/search", params);
    return unwrapPagedList<UserRow>(data, params.page, params.pageSize);
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
    const paged = unwrapPagedList<UserRow>(data, params.page, params.pageSize);

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
  }): Promise<PagedList<AuditLogItem>> {
    const { data } = await http.post("/api/admins/logs/search", params);
    return unwrapPagedList<AuditLogItem>(data, params.page, params.pageSize);
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
