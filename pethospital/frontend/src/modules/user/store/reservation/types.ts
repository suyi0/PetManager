import { DoctorDataItem } from "@/modules/doctor/store/types";
import { State as RootState } from "@/store/types";

export interface ReservateState {
  doctorData: DoctorDataItem[];
  year: string[];
  month: string[];
  day: string[];
  weekday: string[];
  slots: string[][];
}

export type ReservationModuleState = ReservateState;

export interface ReservationRootState extends RootState {
  reservation: ReservationModuleState;
}
