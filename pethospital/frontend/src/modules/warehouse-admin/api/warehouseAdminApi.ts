import axios from "axios";
import { unwrapList } from "@/api/response";
import { authStorage } from "@/core/auth/utils/authStorage";
import { warehouseItemsMock } from "./warehouseAdminMock";
import { WarehouseCreatePayload, WarehouseItem } from "./types";

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

export const warehouseAdminApi = {
  async getAllItems(): Promise<WarehouseItem[]> {
    try {
      const { data } = await http.get("/api/warehouseManager/select");
      const rows = unwrapList<WarehouseItem>(data);
      return rows.length ? rows : warehouseItemsMock;
    } catch {
      return warehouseItemsMock;
    }
  },

  async getItemById(id: number): Promise<WarehouseItem | null> {
    try {
      const { data } = await http.get(
        `/api/warehouseManager/select/dataID/${id}`
      );
      if (data && typeof data === "object" && !Array.isArray(data)) {
        return data as WarehouseItem;
      }
    } catch {
      return warehouseItemsMock.find((item) => item.id === id) || null;
    }
    return warehouseItemsMock.find((item) => item.id === id) || null;
  },

  async getItemsByName(name: string): Promise<WarehouseItem[]> {
    try {
      const { data } = await http.get(
        `/api/warehouseManager/select/item_name/${encodeURIComponent(name)}`
      );
      const rows = unwrapList<WarehouseItem>(data);
      return rows.length
        ? rows
        : warehouseItemsMock.filter((item) => item.item_name.includes(name));
    } catch {
      return warehouseItemsMock.filter((item) => item.item_name.includes(name));
    }
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
