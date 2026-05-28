import { DoctorDataItem } from "@/modules/doctor/api/types";
import {
  PetProfile,
  OrderDetail,
  ReservationOrderRecordItem,
  ReservationSummary,
  OrderSummary,
  ReservationScheduleState,
} from "../api/types";
import { CacheMeta } from "@/app/store/types";

/**
 * 用户端业务缓存状态。
 * 这里只缓存来自后端、用户端高频读取的基础数据。
 * 页面临时选择状态不要放进这里，避免刷新缓存时污染 UI 流程。
 */
export interface UserPortalState {
  petProfiles: PetProfile[];
  reservationDoctors: DoctorDataItem[];
  reservationSchedule: Omit<ReservationScheduleState, "doctorData">;
  reservationRecords: ReservationSummary[];
  orderSummaries: OrderSummary[];
  currentReservationDetail: ReservationOrderRecordItem | null;
  currentOrderDetail: OrderDetail | null;
  petProfilesMeta: CacheMeta;
  reservationDoctorsMeta: CacheMeta;
  reservationScheduleMeta: CacheMeta;
  reservationRecordsMeta: CacheMeta;
  orderSummariesMeta: CacheMeta;
  currentReservationDetailMeta: CacheMeta;
  currentOrderDetailMeta: CacheMeta;
}
