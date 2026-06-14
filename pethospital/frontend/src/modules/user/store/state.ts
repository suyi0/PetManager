import { createCacheMeta } from "@/app/store/state";
import { UserPortalState } from "./types";

/**
 * 创建用户端缓存的初始状态。
 */
export const createUserPortalState = (): UserPortalState => ({
  reservationRecords: [],
  orderSummaries: [],
  currentReservationDetail: null,
  currentOrderDetail: null,
  reservationRecordsMeta: createCacheMeta(),
  orderSummariesMeta: createCacheMeta(),
  currentReservationDetailMeta: createCacheMeta(),
  currentOrderDetailMeta: createCacheMeta(),
});
