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

  async getWorkTimeRecord(): Promise<WorkTimeRecord[]> {
    try {
      const { data } = await http.get("/api/admin/getWorkTimeRecord");
      const rows = unwrapList<WorkTimeRecord>(data);
      return rows.length ? rows : superAdminWorkTimeRecordsMock;
    } catch {
      return superAdminWorkTimeRecordsMock;
    }
  },

  async createDoctor(userID: number): Promise<void> {
    await http.post("/api/admin/createDoctor", { user_id: userID });
  },

  async deleteDoctor(userID: number): Promise<void> {
    await http.post("/api/admin/deleteDoctor", { user_id: userID });
  },

  async createWarehouseAdmin(userID: number): Promise<void> {
    await http.post("/api/admin/createWarehouserManager", { user_id: userID });
  },

  async deleteWarehouseAdmin(userID: number): Promise<void> {
    await http.post("/api/admin/deleteWarehouserManager", { user_id: userID });
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

  async getUsers(): Promise<UserRow[]> {
    try {
      const { data } = await http.get("/api/admin/getUsers");
      const rows = unwrapList<UserRow>(data);
      return rows.length ? rows : superAdminUserRowsMock;
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

  async getLogs(): Promise<{ userLogs: UserLogs[]; systemLogs: SystemLogs[] }> {
    try {
      const { data } = await http.get("/api/admin/getLogs");
      const userLogs = unwrapList<UserLogs>(data?.userLogs);
      const systemLogs = unwrapList<SystemLogs>(data?.systemLogs);

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

  async getHomePageData(): Promise<HomePageSummary> {
    try {
      const { data } = await http.get("/api/admin/homePageGetData");
      const userCount = Number(data?.userCount ?? 0);
      const onlineDoctorCount = Number(data?.onlineDoctorCount ?? 0);
      const logsCount = Number(data?.logsCount ?? data?.logCount ?? 0);

      return {
        userCount,
        onlineDoctorCount,
        logsCount,
      };
    } catch {
      return {
        userCount: 0,
        onlineDoctorCount: 0,
        logsCount: 0,
      };
    }
  },
};
