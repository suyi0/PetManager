import http from "@/api/http";
import { unwrapList } from "@/api/response";
import {
  superAdminSystemLogsMock,
  superAdminUserLogsMock,
  superAdminUserRowsMock,
  superAdminWorkTimeRecordsMock,
} from "./superAdminMock";
import {
  CreateUserPayload,
  HomePageSummary,
  UserRow,
  WorkTimeRecord,
  UserLogs,
  SystemLogs,
} from "./types";

export const superAdminApi = {
  /**
   * 刷新管理员会话
   * @returns 新的访问令牌字符串，或者在请求失败或原令牌无效时抛出错误。
   */
  refreshAdminSession() {
    return http.post("/api/auth/admin/refresh");
  },

  /**
   * 获取考勤记录列表。
   * @returns 获取到的考勤记录列表；如果请求失败，则返回包含模拟数据的列表。
   */
  async getWorkTimeRecord(): Promise<WorkTimeRecord[]> {
    try {
      const { data } = await http.get("/api/admin/getWorkTimeRecord");
      const rows = unwrapList<WorkTimeRecord>(data);
      return rows.length ? rows : superAdminWorkTimeRecordsMock;
    } catch {
      return superAdminWorkTimeRecordsMock;
    }
  },

  async changeDoctorWorkTime(params: {
    user_id: number;
    date: string;
    identifier: "check_in_time" | "check_out_time";
  }): Promise<void> {
    await http.post("/api/admin/changeDoctorWorkTime", params);
  },

  async changeDoctorWorkStatus(params: {
    doctorId: number;
    status: "online" | "offline";
  }): Promise<void> {
    await http.post("/api/admin/changeDoctorWorkStatus", params);
  },

  /**
   * 获取用户列表。
   * @returns 获取到的用户列表；如果请求失败，则返回包含模拟数据的列表。
   */
  async getUsers(): Promise<UserRow[]> {
    try {
      const { data } = await http.get("/api/admin/getUsers");
      const rows = unwrapList<UserRow>(data);
      return rows;
    } catch {
      return superAdminUserRowsMock;
    }
  },

  async createUser(payload: CreateUserPayload): Promise<void> {
    await http.post("/api/admin/createUser", payload);
  },

  async deleteUser(userID: number): Promise<void> {
    await http.post("/api/admin/deleteUser", { user_id: userID });
  },

  /**
   * 获取用户日志和系统日志。
   * @returns 包含用户日志和系统日志的对象；如果请求失败，则返回包含模拟数据的对象。
   */
  async getLogs(): Promise<{ userLogs: UserLogs[]; systemLogs: SystemLogs[] }> {
    try {
      const { data } = await http.get("/api/admin/getLogs");
      const logsPayload = data?.data ?? data;
      const userLogs = unwrapList<UserLogs>(logsPayload?.userLogs);
      const systemLogs = unwrapList<SystemLogs>(logsPayload?.systemLogs);

      return {
        userLogs: userLogs.length ? userLogs : superAdminUserLogsMock,
        systemLogs: systemLogs.length ? systemLogs : superAdminSystemLogsMock,
      };
    } catch {
      return {
        userLogs: superAdminUserLogsMock,
        systemLogs: superAdminSystemLogsMock,
      };
    }
  },

  /**
   * 获取首页摘要数据，包括用户总数、在线医生总数和日志总数。
   * @returns 包含首页摘要数据的对象；如果请求失败，则返回默认值为 0 的对象。
   */
  async homePageGetData(): Promise<HomePageSummary> {
    try {
      const { data } = await http.get("/api/admin/getHomeData");
      const summary = data?.data ?? data;
      const dailyExpense = Number(summary?.dailyExpense ?? 0);
      const dailyCost = Number(summary?.dailyCost ?? 0);
      const dailySales = Number(summary?.dailySales ?? 0);
      const dailyProfit = Number(summary?.dailyProfit ?? 0);
      const userCount = Number(summary?.userCount ?? 0);
      const onlineDoctorCount = Number(summary?.onlineDoctorCount ?? 0);
      const allLogCount = Number(summary?.allLogCount ?? 0);
      const userLogCount = Number(summary?.userLogCount ?? 0);
      const systemLogCount = Number(summary?.systemLogCount ?? 0);

      return {
        dailyExpense,
        dailyCost,
        dailySales,
        dailyProfit,
        userCount,
        onlineDoctorCount,
        allLogCount,
        userLogCount,
        systemLogCount,
      };
    } catch {
      return {
        dailyExpense: 0,
        dailyCost: 0,
        dailySales: 0,
        dailyProfit: 0,
        userCount: 0,
        onlineDoctorCount: 0,
        allLogCount: 0,
        userLogCount: 0,
        systemLogCount: 0,
      };
    }
  },
};
