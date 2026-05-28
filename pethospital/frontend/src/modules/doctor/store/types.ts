import { CacheMeta } from "@/app/store/types";
import {
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderDetailItem,
  OrderRecordItem,
  QueueItem,
  ReservationItem,
  ReservationSummaryItem,
} from "../api/types";

/**
 * 医生端业务缓存状态。
 * 这些数据只保存在当前会话内，关闭页面后会自然失效。
 */
export interface DoctorState {
  dutyStatus: DoctorDutyStatus;
  userProfiles: DoctorUserProfile[];
  queueItems: QueueItem[];
  reservations: ReservationSummaryItem[];
  currentReservationDetail: ReservationItem | null;
  orderRecords: OrderRecordItem[];
  currentOrderDetail: OrderDetailItem | null;
  dutyStatusMeta: CacheMeta;
  userProfilesMeta: CacheMeta;
  queueItemsMeta: CacheMeta;
  reservationsMeta: CacheMeta;
  currentReservationDetailMeta: CacheMeta;
  orderRecordsMeta: CacheMeta;
  currentOrderDetailMeta: CacheMeta;
}
