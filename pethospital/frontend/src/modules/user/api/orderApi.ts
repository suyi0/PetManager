import http from "@/api/http";

export const orderApi = {
  getOrders(params: {
    name?: string | null;
    phone?: string | null;
    email?: string | null;
  }) {
    return http.get("/api/order/getrecord", { params });
  },

  getOrderList() {
    return http.get("/api/order/getOrderList");
  },
};
