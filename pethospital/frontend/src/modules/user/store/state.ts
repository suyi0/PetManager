import { createCacheMeta } from "@/app/store/state";
import { UserPortalState } from "./types";

/**
 * 创建用户端缓存的初始状态。
 */
export const createUserPortalState = (): UserPortalState => ({
  petProfiles: [],
  reservationDoctors: [],
  reservationSchedule: {
    year: [],
    month: [],
    day: [],
    weekday: [],
    slots: [],
  },
  reservationRecords: [],
  orderSummaries: [],
  currentReservationDetail: null,
  currentOrderDetail: null,
  petProfilesMeta: createCacheMeta(),
  reservationDoctorsMeta: createCacheMeta(),
  reservationScheduleMeta: createCacheMeta(),
  reservationRecordsMeta: createCacheMeta(),
  orderSummariesMeta: createCacheMeta(),
  currentReservationDetailMeta: createCacheMeta(),
  currentOrderDetailMeta: createCacheMeta(),
});
