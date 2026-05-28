import { DoctorDataItem } from "@/modules/doctor/api/types";
import {
  PetProfile,
  OrderDetail,
  OrderSummary,
  ReservationOrderRecordItem,
  ReservationSummary,
  ReservationScheduleState,
} from "../api/types";

const USER_PORTAL_CACHE_KEYS = {
  petProfiles: "user-portal:pet-profiles:cache",
  reservationDoctors: "user-portal:reservation-doctors:cache",
  reservationSchedule: "user-portal:reservation-schedule:cache",
  reservationRecords: "user-portal:reservation-records:cache",
  currentReservationDetail: "user-portal:current-reservation-detail:cache",
  orderSummaries: "user-portal:order-summaries:cache",
  currentOrderDetail: "user-portal:current-order-detail:cache",
};

const readJsonCache = <T>(key: string): T | null => {
  const rawValue = localStorage.getItem(key);

  if (!rawValue) {
    return null;
  }

  try {
    return JSON.parse(rawValue) as T;
  } catch {
    localStorage.removeItem(key);
    return null;
  }
};

const saveJsonCache = <T>(key: string, value: T) => {
  localStorage.setItem(key, JSON.stringify(value));
};

const readArrayCache = <T>(key: string): T[] | null => {
  const cachedValue = readJsonCache<unknown>(key);

  return Array.isArray(cachedValue) ? (cachedValue as T[]) : null;
};

/**
 * 从本地缓存读取用户端宠物档案。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserPetProfilesCache = () =>
  readArrayCache<PetProfile>(USER_PORTAL_CACHE_KEYS.petProfiles);

/**
 * 写入用户端宠物档案本地缓存。
 */
export const saveUserPetProfilesCache = (pets: PetProfile[]) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.petProfiles, pets);
};

/**
 * 从本地缓存读取用户端预约医生列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserReservationDoctorsCache = () =>
  readArrayCache<DoctorDataItem>(USER_PORTAL_CACHE_KEYS.reservationDoctors);

/**
 * 写入用户端预约医生列表本地缓存。
 */
export const saveUserReservationDoctorsCache = (doctors: DoctorDataItem[]) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.reservationDoctors, doctors);
};

/**
 * 从本地缓存读取用户端预约时间表。
 * 缓存不存在或格式异常时返回 null。
 */
export const readUserReservationScheduleCache = () =>
  readJsonCache<Omit<ReservationScheduleState, "doctorData">>(
    USER_PORTAL_CACHE_KEYS.reservationSchedule
  );

/**
 * 写入用户端预约时间表本地缓存。
 */
export const saveUserReservationScheduleCache = (
  schedule: Omit<ReservationScheduleState, "doctorData">
) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.reservationSchedule, schedule);
};

/**
 * 从本地缓存读取用户端订单摘要。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserOrderSummariesCache = () =>
  readArrayCache<OrderSummary>(USER_PORTAL_CACHE_KEYS.orderSummaries);

/**
 * 写入用户端订单摘要本地缓存。
 */
export const saveUserOrderSummariesCache = (summaries: OrderSummary[]) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.orderSummaries, summaries);
};

/**
 * 从本地缓存读取当前选中的完整订单信息。
 * 该缓存只保存一条记录，进入新订单详情时会覆盖旧记录。
 */
export const readUserCurrentOrderDetailCache = () =>
  readJsonCache<OrderDetail>(USER_PORTAL_CACHE_KEYS.currentOrderDetail);

/**
 * 写入当前选中的完整订单信息，并覆盖上一条详情缓存。
 */
export const saveUserCurrentOrderDetailCache = (detail: OrderDetail) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.currentOrderDetail, detail);
};

/**
 * 从本地缓存读取用户端预约记录。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserReservationRecordsCache = () =>
  readArrayCache<ReservationSummary>(USER_PORTAL_CACHE_KEYS.reservationRecords);

/**
 * 写入用户端预约记录本地缓存。
 */
export const saveUserReservationRecordsCache = (
  records: ReservationSummary[]
) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.reservationRecords, records);
};

/**
 * 从本地缓存读取当前选中的完整预约信息。
 * 该缓存只保存一条记录，进入新预约详情时会覆盖旧记录。
 */
export const readUserCurrentReservationDetailCache = () =>
  readJsonCache<ReservationOrderRecordItem>(
    USER_PORTAL_CACHE_KEYS.currentReservationDetail
  );

/**
 * 写入当前选中的完整预约信息，并覆盖上一条详情缓存。
 */
export const saveUserCurrentReservationDetailCache = (
  detail: ReservationOrderRecordItem
) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.currentReservationDetail, detail);
};

/**
 * 清空用户端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧业务数据。
 */
export const clearUserPortalDataCache = () => {
  Object.values(USER_PORTAL_CACHE_KEYS).forEach((key) => {
    localStorage.removeItem(key);
  });
};
