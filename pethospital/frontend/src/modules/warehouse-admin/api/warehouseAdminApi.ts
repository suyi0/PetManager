import axios from "axios";
import { authStorage } from "@/core/auth/utils/authStorage";
import {
  ApiListResponse,
  WarehouseCreatePayload,
  WarehouseItem,
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

const unwrapList = <T>(payload: unknown): T[] => {
  if (Array.isArray(payload)) return payload as T[];
  if (payload && typeof payload === "object") {
    const maybe = payload as ApiListResponse<T>;
    if (Array.isArray(maybe.data)) return maybe.data;
  }
  return [];
};

export const warehouseAdminApi = {
  async getAllItems(): Promise<WarehouseItem[]> {
    const { data } = await http.get("/api/warehouseManager/select");
    return unwrapList<WarehouseItem>(data);
  },

  async getItemById(id: number): Promise<WarehouseItem | null> {
    const { data } = await http.get(
      `/api/warehouseManager/select/dataID/${id}`
    );
    if (data && typeof data === "object" && !Array.isArray(data)) {
      return data as WarehouseItem;
    }
    return null;
  },

  async getItemsByName(name: string): Promise<WarehouseItem[]> {
    const { data } = await http.get(
      `/api/warehouseManager/select/item_name/${encodeURIComponent(name)}`
    );
    return unwrapList<WarehouseItem>(data);
  },

  async createItem(payload: WarehouseCreatePayload): Promise<void> {
    await http.post("/api/warehouseManager/upload", payload);
  },

  async updateItem(id: number, payload: WarehouseCreatePayload): Promise<void> {
    await http.patch(`/api/warehouseManager/updata/${id}`, payload);
  },

  async deleteItem(id: number): Promise<void> {
    await http.delete(`/api/warehouseManager/delete/${id}`);
  },
};
