import http from "@/api/http";
import { DoctorDutyStatus } from "./types";

const unwrapMessage = (payload: unknown, fallback: string): string => {
  if (typeof payload === "string") {
    return payload;
  }

  if (payload && typeof payload === "object") {
    const candidate = payload as { message?: unknown; data?: unknown };

    if (typeof candidate.message === "string") {
      return candidate.message;
    }

    if (typeof candidate.data === "string") {
      return candidate.data;
    }
  }

  return fallback;
};

export const doctorApi = {
  async getDutyStatus(): Promise<DoctorDutyStatus> {
    const { data } = await http.get("/api/doctor/dutyStatus");

    if (data && typeof data === "object" && !Array.isArray(data)) {
      return data as DoctorDutyStatus;
    }

    return {
      is_online: false,
      date: "",
      check_in_time: "",
      check_out_time: "",
      status: "offline",
    };
  },

  async online(): Promise<string> {
    const { data } = await http.post("/api/doctor/online");
    return unwrapMessage(data, "签到成功!");
  },

  async offline(): Promise<string> {
    const { data } = await http.post("/api/doctor/offline");
    return unwrapMessage(data, "签退成功!");
  },
};
