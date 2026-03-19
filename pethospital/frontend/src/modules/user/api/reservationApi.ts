import http from "@/api/http";

export const reservationApi = {
  getScheduleData() {
    return http.get("/api/reservate/getData");
  },

  getDoctors() {
    return http.get("/api/reservate/getDoctor");
  },

  createReservationRecord(payload: {
    name?: string | null;
    phone?: string | null;
    email?: string | null;
    doctorId: number;
    date: string;
    slot: string;
  }) {
    return http.post("/api/reservate/record", payload);
  },

  getReservations(params: {
    name?: string | null;
    phone?: string | null;
    email?: string | null;
  }) {
    return http.get("/api/reservate/getrecord", { params });
  },
};
