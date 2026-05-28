import { createCacheMeta } from "@/app/store/state";
import { DoctorDutyStatus } from "../api/types";
import { DoctorState } from "./types";

const createDutyStatus = (): DoctorDutyStatus => ({
  is_online: false,
  date: "",
  check_in_time: "",
  check_out_time: "",
  status: "offline",
});

/**
 * 创建医生端缓存状态。
 */
export const createDoctorState = (): DoctorState => ({
  dutyStatus: createDutyStatus(),
  userProfiles: [],
  queueItems: [],
  reservations: [],
  currentReservationDetail: null,
  orderRecords: [],
  currentOrderDetail: null,
  dutyStatusMeta: createCacheMeta(),
  userProfilesMeta: createCacheMeta(),
  queueItemsMeta: createCacheMeta(),
  reservationsMeta: createCacheMeta(),
  currentReservationDetailMeta: createCacheMeta(),
  orderRecordsMeta: createCacheMeta(),
  currentOrderDetailMeta: createCacheMeta(),
});
