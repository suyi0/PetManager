import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { unwrapPagedList } from "@/shared/utils/pagedList";
import { PagedList, UserRow } from "@/modules/super-admin/api/types";

export const personnelApi = {
  refreshSession() {
    return http.post("/api/admins/session-renewals");
  },

  async getUsers(): Promise<UserRow[]> {
    const { data } = await http.get("/api/admins/users");
    return unwrapList<UserRow>(data);
  },

  async searchUsers(params: {
    keyword: string;
    page: number;
    pageSize: number;
  }): Promise<PagedList<UserRow>> {
    const { data } = await http.post("/api/admins/users/search", {
      ...params,
      role: "all",
    });
    return unwrapPagedList<UserRow>(data, params);
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
