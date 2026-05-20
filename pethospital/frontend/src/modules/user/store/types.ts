import { DoctorDataItem } from "@/modules/doctor/api/types";
import {
  OrderRecordItem,
  OrderSummary,
  ReservationScheduleState,
} from "../api/types";
import { CacheMeta } from "@/app/store/types";

export interface PetProfile {
  id: string;
  name: string;
  species: string;
  breed: string;
  age: string;
  gender: string;
  neutered: string;
  vaccineStatus: string;
  preference: string;
  notes: string;
}

/**
 * 用户端业务缓存状态。
 * 这里只缓存用户端高频读取的基础数据，关闭页面后会自然失效。
 */
export interface UserPortalState {
  petProfiles: PetProfile[];
  reservationDoctors: DoctorDataItem[];
  reservationSchedule: Omit<ReservationScheduleState, "doctorData">;
  orderRecords: OrderRecordItem[];
  reservationRecords: OrderRecordItem[];
  orderSummaries: OrderSummary[];
  petProfilesMeta: CacheMeta;
  reservationDoctorsMeta: CacheMeta;
  reservationScheduleMeta: CacheMeta;
  orderRecordsMeta: CacheMeta;
  reservationRecordsMeta: CacheMeta;
  orderSummariesMeta: CacheMeta;
}
