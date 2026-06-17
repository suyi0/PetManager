import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { PagedList, UserRow } from "@/modules/super-admin/api/types";

const unwrapPagedList = <T>(
  payload: unknown,
  page: number,
  pageSize: number
): PagedList<T> => {
  const data = (payload as { data?: unknown })?.data ?? payload;
  const source = data as {
    items?: unknown;
    total?: unknown;
    page?: unknown;
    pageSize?: unknown;
  };
  const items = unwrapList<T>(source?.items);

  return {
    items,
    total: Number(source?.total ?? items.length),
    page: Number(source?.page ?? page),
    pageSize: Number(source?.pageSize ?? pageSize),
  };
};

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
    return unwrapPagedList<UserRow>(data, params.page, params.pageSize);
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
