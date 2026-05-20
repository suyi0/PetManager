import { DoctorDataItem } from "@/modules/doctor/api/types";
import {
  OrderRecordItem,
  OrderSummary,
  ReservationScheduleState,
} from "../api/types";
import { PetProfile } from "../store/types";

const USER_PORTAL_CACHE_KEYS = {
  petProfiles: "user-portal:pet-profiles:cache",
  reservationDoctors: "user-portal:reservation-doctors:cache",
  reservationSchedule: "user-portal:reservation-schedule:cache",
  orderRecords: "user-portal:order-records:cache",
  reservationRecords: "user-portal:reservation-records:cache",
  orderSummaries: "user-portal:order-summaries:cache",
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
 * 从本地缓存读取用户端普通订单记录。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserOrderRecordsCache = () =>
  readArrayCache<OrderRecordItem>(USER_PORTAL_CACHE_KEYS.orderRecords);

/**
 * 写入用户端普通订单记录本地缓存。
 */
export const saveUserOrderRecordsCache = (records: OrderRecordItem[]) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.orderRecords, records);
};

/**
 * 从本地缓存读取用户端预约记录。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readUserReservationRecordsCache = () =>
  readArrayCache<OrderRecordItem>(USER_PORTAL_CACHE_KEYS.reservationRecords);

/**
 * 写入用户端预约记录本地缓存。
 */
export const saveUserReservationRecordsCache = (records: OrderRecordItem[]) => {
  saveJsonCache(USER_PORTAL_CACHE_KEYS.reservationRecords, records);
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
 * 清空用户端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧业务数据。
 */
export const clearUserPortalDataCache = () => {
  Object.values(USER_PORTAL_CACHE_KEYS).forEach((key) => {
    localStorage.removeItem(key);
  });
};
