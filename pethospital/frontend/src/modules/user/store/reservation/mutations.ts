import { DoctorDataItem } from "@/modules/doctor/store/types";
import { MutationTree } from "vuex";
import { authStorage } from "@/core/auth/utils/authStorage";
import { ReservationModuleState, ReservateState } from "./types";

export const reservationMutations: MutationTree<ReservationModuleState> = {
  setReservate(state, reservate: Omit<ReservateState, "doctorData">) {
    state.year = reservate.year;
    state.month = reservate.month;
    state.day = reservate.day;
    state.weekday = reservate.weekday;
    state.slots = reservate.slots;

    authStorage.saveReservate(reservate);
  },

  setDoctorData(state, doctorData: DoctorDataItem[]) {
    state.doctorData = doctorData;
    authStorage.saveDoctorData(doctorData);
  },
};
