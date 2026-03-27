import axios from "axios";
import { authStorage } from "@/core/auth/utils/authStorage";
import {
  ApiListResponse,
  CreateUserPayload,
  UserRow,
  WorkTimeRecord,
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

// 统一处理列表数据
const unwrapList = <T>(payload: unknown): T[] => {
  if (Array.isArray(payload)) return payload as T[]; // 假设 payload 是一个数组,直接返回数组
  if (payload && typeof payload === "object") {
    // 假设 payload 是一个对象,尝试将其进行 ApiListResponse<T>函数转换为数组
    const maybe = payload as ApiListResponse<T>;
    if (Array.isArray(maybe.data)) return maybe.data;
  }
  return []; // 其他情况返回一个空数组
};

export const superAdminApi = {
  refreshAdminSession() {
    return http.post("/api/auth/admin/refresh");
  },

  async getWorkTimeRecord(): Promise<WorkTimeRecord[]> {
    const { data } = await http.get("/api/admin/getWorkTimeRecord");
    return unwrapList<WorkTimeRecord>(data);
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
    const { data } = await http.get("/api/allUser/getdata");
    return unwrapList<UserRow>(data);
  },

  async createUser(payload: CreateUserPayload): Promise<void> {
    await http.post("/api/admin/createUser", payload);
  },

  async deleteUser(userID: number): Promise<void> {
    await http.post("/api/admin/deleteUser", { user_id: userID });
  },
};
