import { DoctorDataItem } from "@/modules/doctor/api/types";
import {
  PetProfile,
  OrderDetail,
  OrderSummary,
  ReservationOrderRecordItem,
  ReservationSummary,
  ReservationScheduleState,
} from "../api/types";
import { createVersionedLocalCacheAccessors } from "@/shared/utils/versionedLocalCache";

const USER_PORTAL_CACHE_OPTIONS = {
  version: 1,
  ttlMs: 1000 * 60 * 60 * 24 * 30,
};

const USER_PORTAL_CACHE_KEYS = {
  petProfiles: "user-portal:pet-profiles:cache",
  reservationDoctors: "user-portal:reservation-doctors:cache",
  reservationSchedule: "user-portal:reservation-schedule:cache",
  reservationRecords: "user-portal:reservation-records:cache",
  currentReservationDetail: "user-portal:current-reservation-detail:cache",
  orderSummaries: "user-portal:order-summaries:cache",
  currentOrderDetail: "user-portal:current-order-detail:cache",
};

const userPortalCache = createVersionedLocalCacheAccessors(
  USER_PORTAL_CACHE_OPTIONS
);

/**
 * 从本地缓存读取用户端宠物档案。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserPetProfilesCache = () =>
  userPortalCache.readArray<PetProfile>(USER_PORTAL_CACHE_KEYS.petProfiles);

/**
 * 写入用户端宠物档案本地缓存。
 */
export const saveUserPetProfilesCache = (pets: PetProfile[]) => {
  userPortalCache.save(USER_PORTAL_CACHE_KEYS.petProfiles, pets);
};

/**
 * 从本地缓存读取用户端预约医生列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserReservationDoctorsCache = () =>
  userPortalCache.readArray<DoctorDataItem>(
    USER_PORTAL_CACHE_KEYS.reservationDoctors
  );

/**
 * 写入用户端预约医生列表本地缓存。
 */
export const saveUserReservationDoctorsCache = (doctors: DoctorDataItem[]) => {
  userPortalCache.save(USER_PORTAL_CACHE_KEYS.reservationDoctors, doctors);
};

/**
 * 从本地缓存读取用户端预约时间表。
 * 缓存不存在或格式异常时返回 null。
 */
export const readUserReservationScheduleCache = () =>
  userPortalCache.read<Omit<ReservationScheduleState, "doctorData">>(
    USER_PORTAL_CACHE_KEYS.reservationSchedule
  );

/**
 * 写入用户端预约时间表本地缓存。
 */
export const saveUserReservationScheduleCache = (
  schedule: Omit<ReservationScheduleState, "doctorData">
) => {
  userPortalCache.save(USER_PORTAL_CACHE_KEYS.reservationSchedule, schedule);
};

/**
 * 从本地缓存读取用户端订单摘要。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserOrderSummariesCache = () =>
  userPortalCache.readArray<OrderSummary>(
    USER_PORTAL_CACHE_KEYS.orderSummaries
  );

/**
 * 写入用户端订单摘要本地缓存。
 */
export const saveUserOrderSummariesCache = (summaries: OrderSummary[]) => {
  userPortalCache.save(USER_PORTAL_CACHE_KEYS.orderSummaries, summaries);
};

/**
 * 从本地缓存读取当前选中的完整订单信息。
 * 该缓存只保存一条记录，进入新订单详情时会覆盖旧记录。
 */
export const readUserCurrentOrderDetailCache = () =>
  userPortalCache.read<OrderDetail>(USER_PORTAL_CACHE_KEYS.currentOrderDetail);

/**
 * 写入当前选中的完整订单信息，并覆盖上一条详情缓存。
 */
export const saveUserCurrentOrderDetailCache = (detail: OrderDetail) => {
  userPortalCache.save(USER_PORTAL_CACHE_KEYS.currentOrderDetail, detail);
};

/**
 * 清空当前选中的完整订单信息缓存。
 */
export const clearUserCurrentOrderDetailCache = () => {
  userPortalCache.remove(USER_PORTAL_CACHE_KEYS.currentOrderDetail);
};

/**
 * 从本地缓存读取用户端预约记录。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserReservationRecordsCache = () =>
  userPortalCache.readArray<ReservationSummary>(
    USER_PORTAL_CACHE_KEYS.reservationRecords
  );

/**
 * 写入用户端预约记录本地缓存。
 */
export const saveUserReservationRecordsCache = (
  records: ReservationSummary[]
) => {
  userPortalCache.save(USER_PORTAL_CACHE_KEYS.reservationRecords, records);
};

/**
 * 从本地缓存读取当前选中的完整预约信息。
 * 该缓存只保存一条记录，进入新预约详情时会覆盖旧记录。
 */
export const readUserCurrentReservationDetailCache = () =>
  userPortalCache.read<ReservationOrderRecordItem>(
    USER_PORTAL_CACHE_KEYS.currentReservationDetail
  );

/**
 * 写入当前选中的完整预约信息，并覆盖上一条详情缓存。
 */
export const saveUserCurrentReservationDetailCache = (
  detail: ReservationOrderRecordItem
) => {
  userPortalCache.save(USER_PORTAL_CACHE_KEYS.currentReservationDetail, detail);
};

/**
 * 清空当前选中的完整预约信息缓存。
 */
export const clearUserCurrentReservationDetailCache = () => {
  userPortalCache.remove(USER_PORTAL_CACHE_KEYS.currentReservationDetail);
};

/**
 * 清空用户端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧业务数据。
 */
export const clearUserPortalDataCache = () => {
  userPortalCache.clearAll(Object.values(USER_PORTAL_CACHE_KEYS));
};
