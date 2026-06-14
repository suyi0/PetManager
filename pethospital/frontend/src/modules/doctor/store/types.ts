import { CacheMeta } from "@/app/store/types";
import {
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderDetailItem,
  OrderSummaryItem,
  QueueItem,
  ReservationItem,
  ReservationSummaryItem,
} from "../api/types";
import { DoctorOrderDraft } from "../utils/orderDrafts";

/**
 * 医生端业务缓存状态。
 * 这些数据只保存在当前会话内，关闭页面后会自然失效。
 */
export interface DoctorState {
  dutyStatus: DoctorDutyStatus;
  currentUserProfile: DoctorUserProfile | null;
  queueItems: QueueItem[];
  reservations: ReservationSummaryItem[];
  currentReservationDetail: ReservationItem | null;
  orderRecords: OrderSummaryItem[];
  currentOrderDetail: OrderDetailItem | null;
  orderDrafts: Record<string, DoctorOrderDraft>;
  dutyStatusMeta: CacheMeta;
  currentUserProfileMeta: CacheMeta;
  queueItemsMeta: CacheMeta;
  reservationsMeta: CacheMeta;
  currentReservationDetailMeta: CacheMeta;
  orderRecordsMeta: CacheMeta;
  currentOrderDetailMeta: CacheMeta;
}
