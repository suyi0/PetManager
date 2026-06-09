import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { warehouseItemsMock } from "./warehouseAdminMock";
import { WarehouseCreatePayload, WarehouseItem } from "./types";

export const warehouseAdminApi = {
  async select(): Promise<WarehouseItem[]> {
    try {
      const { data } = await http.get("/api/warehouse-managers/items");
      const rows = unwrapList<WarehouseItem>(data);
      return rows;
    } catch {
      return warehouseItemsMock;
    }
  },

  async selectDataID(id: number): Promise<WarehouseItem | null> {
    try {
      const { data } = await http.get(
        `/api/warehouse-managers/items/data-id/${id}`
      );
      if (data && typeof data === "object" && !Array.isArray(data)) {
        return data as WarehouseItem;
      }
    } catch {
      return warehouseItemsMock.find((item) => item.id === id) || null;
    }
    return warehouseItemsMock.find((item) => item.id === id) || null;
  },

  async selectItemName(name: string): Promise<WarehouseItem[]> {
    try {
      const { data } = await http.get(
        `/api/warehouse-managers/items/item-name/${encodeURIComponent(name)}`
      );
      const rows = unwrapList<WarehouseItem>(data);
      return rows;
    } catch {
      return warehouseItemsMock.filter((item) => item.item_name.includes(name));
    }
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
