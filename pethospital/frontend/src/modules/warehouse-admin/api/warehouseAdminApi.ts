import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { unwrapPagedList } from "@/shared/utils/pagedList";
import { WarehouseCreatePayload, WarehouseItem } from "./types";

interface WarehouseSearchResult {
  items: WarehouseItem[];
  total: number;
  page: number;
  pageSize: number;
}

const unwrapData = <T>(payload: unknown): T | null => {
  if (payload && typeof payload === "object" && "data" in payload) {
    return (payload as { data?: T }).data ?? null;
  }

  return (payload as T) ?? null;
};

const normalizeWarehouseSearchResult = (
  payload: unknown,
  fallback: { page: number; pageSize: number }
): WarehouseSearchResult => {
  return unwrapPagedList<WarehouseItem>(payload, fallback);
};

export const warehouseAdminApi = {
  async select(): Promise<WarehouseItem[]> {
    const { data } = await http.get("/api/warehouse-managers/items");
    return unwrapList<WarehouseItem>(data);
  },

  async list(params: {
    keyword: string;
    itemType: string;
    sortKey: string;
    page: number;
    pageSize: number;
  }): Promise<WarehouseSearchResult> {
    const { data } = await http.get("/api/warehouse-managers/items", {
      params,
    });
    return normalizeWarehouseSearchResult(data, params);
  },

  async search(params: {
    keyword: string;
    itemType: string;
    sortKey: string;
    page: number;
    pageSize: number;
  }): Promise<WarehouseSearchResult> {
    const { data } = await http.post(
      "/api/warehouse-managers/items/search",
      params
    );
    return normalizeWarehouseSearchResult(data, params);
  },

  async selectDataID(id: number): Promise<WarehouseItem | null> {
    const { data } = await http.get(
      `/api/warehouse-managers/items/data-id/${id}`
    );
    const item = unwrapData<WarehouseItem>(data);
    if (item && typeof item === "object" && !Array.isArray(item)) {
      return item;
    }

    return null;
  },

  async selectItemName(name: string): Promise<WarehouseItem[]> {
    const { data } = await http.get(
      `/api/warehouse-managers/items/item-name/${encodeURIComponent(name)}`
    );
    return unwrapList<WarehouseItem>(data);
  },

  async upload(payload: WarehouseCreatePayload): Promise<void> {
    await http.post("/api/warehouse-managers/items", payload);
  },

  async updata(id: number, payload: WarehouseCreatePayload): Promise<void> {
    await http.patch(`/api/warehouse-managers/items/${id}`, {
      id,
      ...payload,
    });
  },

  async delete(id: number): Promise<void> {
    await http.delete("/api/warehouse-managers/item-deletions", {
      data: { dataID: id },
    });
  },
};
