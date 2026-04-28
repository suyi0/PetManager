import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { UserRow } from "@/modules/super-admin/api/types";
import { superAdminUserRowsMock } from "@/modules/super-admin/api/superAdminMock";

export const personnelApi = {
  refreshSession() {
    return http.post("/api/auth/admin/refresh");
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

  async createDoctor(userID: number): Promise<void> {
    await http.post("/api/personnel/createDoctor", { user_id: userID });
  },

  async deleteDoctor(userID: number): Promise<void> {
    await http.post("/api/personnel/deleteDoctor", { user_id: userID });
  },

  async createWarehouseAdmin(userID: number): Promise<void> {
    await http.post("/api/personnel/createWarehouserManager", {
      user_id: userID,
    });
  },

  async deleteWarehouseAdmin(userID: number): Promise<void> {
    await http.post("/api/personnel/deleteWarehouserManager", {
      user_id: userID,
    });
  },
};
