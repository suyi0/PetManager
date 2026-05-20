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
  orderRecords: [],
  reservationRecords: [],
  orderSummaries: [],
  petProfilesMeta: createCacheMeta(),
  reservationDoctorsMeta: createCacheMeta(),
  reservationScheduleMeta: createCacheMeta(),
  orderRecordsMeta: createCacheMeta(),
  reservationRecordsMeta: createCacheMeta(),
  orderSummariesMeta: createCacheMeta(),
});
