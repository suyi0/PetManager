import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { UserRow } from "@/modules/super-admin/api/types";

export const personnelApi = {
  refreshSession() {
    return http.post("/api/admins/session-renewals");
  },

  async getUsers(): Promise<UserRow[]> {
    const { data } = await http.get("/api/admins/users");
    return unwrapList<UserRow>(data);
  },

  async createDoctor(userID: number): Promise<void> {
    await http.post("/api/personnel/doctor-assignments", { user_id: userID });
  },

  async deleteDoctor(userID: number): Promise<void> {
    await http.post("/api/personnel/doctor-removals", { user_id: userID });
  },

  async createWarehouserManager(userID: number): Promise<void> {
    await http.post("/api/personnel/warehouse-manager-assignments", {
      user_id: userID,
    });
  },

  async deleteWarehouserManager(userID: number): Promise<void> {
    await http.post("/api/personnel/warehouse-manager-removals", {
      user_id: userID,
    });
  },
};
