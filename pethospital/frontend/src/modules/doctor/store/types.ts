import { CacheMeta } from "@/app/store/types";
import {
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderRecordItem,
  QueueItem,
  ReservationItem,
} from "../api/types";

/**
 * 医生端业务缓存状态。
 * 这些数据只保存在当前会话内，关闭页面后会自然失效。
 */
export interface DoctorState {
  dutyStatus: DoctorDutyStatus;
  userProfiles: DoctorUserProfile[];
  queueItems: QueueItem[];
  reservations: ReservationItem[];
  orderRecords: OrderRecordItem[];
  dutyStatusMeta: CacheMeta;
  userProfilesMeta: CacheMeta;
  queueItemsMeta: CacheMeta;
  reservationsMeta: CacheMeta;
  orderRecordsMeta: CacheMeta;
}
