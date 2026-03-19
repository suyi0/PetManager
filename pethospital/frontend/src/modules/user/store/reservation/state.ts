import { authStorage } from "@/core/auth/utils/authStorage";
import { ReservationModuleState } from "./types";

export const createReservationState = (): ReservationModuleState => {
  const persistedReservate = authStorage.loadReservate();

  return {
    doctorData: persistedReservate.doctorData,
    year: persistedReservate.year,
    month: persistedReservate.month,
    day: persistedReservate.day,
    weekday: persistedReservate.weekday,
    slots: persistedReservate.slots,
  };
};
