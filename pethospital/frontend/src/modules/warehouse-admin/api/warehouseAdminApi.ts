import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { WarehouseCreatePayload, WarehouseItem } from "./types";

const unwrapData = <T>(payload: unknown): T | null => {
  if (payload && typeof payload === "object" && "data" in payload) {
    return (payload as { data?: T }).data ?? null;
  }

  return (payload as T) ?? null;
};

export const warehouseAdminApi = {
  async select(): Promise<WarehouseItem[]> {
    const { data } = await http.get("/api/warehouse-managers/items");
    return unwrapList<WarehouseItem>(data);
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
