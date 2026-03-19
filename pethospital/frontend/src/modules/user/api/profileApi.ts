import http from "@/api/http";

export const profileApi = {
  saveUserData(payload: {
    name: string;
    phone?: string | null;
    email?: string | null;
    birthday?: string | null;
    address_id?: string | null;
    address?: string | null;
    headImage?: string | null | undefined;
  }) {
    return http.post("/api/upload/form", payload);
  },
};
