import axios from "axios";
import { unwrapList } from "@/api/response";
import { authStorage } from "@/core/auth/utils/authStorage";
import {
  superAdminSystemLogsMock,
  superAdminUserLogsMock,
  superAdminUserRowsMock,
  superAdminWorkTimeRecordsMock,
} from "./superAdminMock";
import {
  CreateUserPayload,
  UserRow,
  WorkTimeRecord,
  UserLogs,
  SystemLogs,
} from "./types";

const http = axios.create({
  baseURL: "",
  timeout: 12000,
});

http.interceptors.request.use((config) => {
  const token = authStorage.getToken();
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

export const superAdminApi = {
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
};
