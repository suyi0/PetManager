import {
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderDetailItem,
  OrderSummaryItem,
  QueueItem,
  ReservationItem,
  ReservationSummaryItem,
} from "../api/types";
import { createVersionedLocalCacheAccessors } from "@/shared/utils/versionedLocalCache";

const DOCTOR_CACHE_OPTIONS = {
  version: 1,
  ttlMs: 1000 * 60 * 60 * 24 * 30,
};

const DOCTOR_CACHE_KEYS = {
  dutyStatus: "doctor:duty-status:cache",
  currentUserProfile: "doctor:current-user-profile:cache",
  queueItems: "doctor:queue-items:cache",
  reservations: "doctor:reservations:cache",
  currentReservationDetail: "doctor:current-reservation-detail:cache",
  orderRecords: "doctor:order-records:cache",
  currentOrderDetail: "doctor:current-order-detail:cache",
};

const doctorCache = createVersionedLocalCacheAccessors(DOCTOR_CACHE_OPTIONS);

/**
 * 标准化订单记录列表，去除重复项。
 * @param records 接收的订单记录列表
 * @returns 标准化后的订单记录列表
 */
const normalizeOrderRecords = (records: OrderSummaryItem[]) => {
  const uniqueRecords = new Map<number, OrderSummaryItem>();

  records.forEach((record) => {
    uniqueRecords.set(Number(record.id), {
      ...record,
      id: Number(record.id),
    });
  });

  return [...uniqueRecords.values()];
};

/**
 * 从本地缓存读取医生值班状态。
 * 缓存不存在或格式异常时返回 null，调用方可继续请求后端。
 */
export const readDoctorDutyStatusCache = () =>
  doctorCache.read<DoctorDutyStatus>(DOCTOR_CACHE_KEYS.dutyStatus);

/**
 * 写入医生值班状态本地缓存。
 */
export const saveDoctorDutyStatusCache = (status: DoctorDutyStatus) => {
  doctorCache.save(DOCTOR_CACHE_KEYS.dutyStatus, status);
};

/**
 * 从本地缓存读取当前选中的用户详情。
 * 该缓存只保存一条记录，进入新用户详情时会覆盖旧记录。
 */
export const readDoctorCurrentUserProfileCache = () =>
  doctorCache.read<DoctorUserProfile>(DOCTOR_CACHE_KEYS.currentUserProfile);

/**
 * 写入当前选中的用户详情，并覆盖上一条详情缓存。
 */
export const saveDoctorCurrentUserProfileCache = (
  profile: DoctorUserProfile
) => {
  doctorCache.save(DOCTOR_CACHE_KEYS.currentUserProfile, profile);
};

/**
 * 从本地缓存读取待接诊队列列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readDoctorQueueItemsCache = () =>
  doctorCache.readArray<QueueItem>(DOCTOR_CACHE_KEYS.queueItems);

/**
 * 写入待接诊队列列表本地缓存。
 */
export const saveDoctorQueueItemsCache = (queueItems: QueueItem[]) => {
  doctorCache.save(DOCTOR_CACHE_KEYS.queueItems, queueItems);
};

/**
 * 从本地缓存读取医生端预约列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readDoctorReservationsCache = () =>
  doctorCache.readArray<ReservationSummaryItem>(DOCTOR_CACHE_KEYS.reservations);

/**
 * 写入医生端预约列表本地缓存。
 */
export const saveDoctorReservationsCache = (
  reservations: ReservationSummaryItem[]
) => {
  doctorCache.save(DOCTOR_CACHE_KEYS.reservations, reservations);
};

/**
 * 从本地缓存读取当前选中的完整预约信息。
 * 该缓存只保存一条记录，进入新预约详情时会覆盖旧记录。
 */
export const readDoctorCurrentReservationDetailCache = () =>
  doctorCache.read<ReservationItem>(DOCTOR_CACHE_KEYS.currentReservationDetail);

/**
 * 写入当前选中的完整预约信息，并覆盖上一条详情缓存。
 */
export const saveDoctorCurrentReservationDetailCache = (
  detail: ReservationItem
) => {
  doctorCache.save(DOCTOR_CACHE_KEYS.currentReservationDetail, detail);
};

/**
 * 从本地缓存读取医生端订单记录列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readDoctorOrderRecordCache = () =>
  doctorCache.readArray<OrderSummaryItem>(DOCTOR_CACHE_KEYS.orderRecords);

/**
 * 将后端返回的医生端订单记录列表写入浏览器本地缓存。
 * 写入前会按订单 id 去重，避免重复渲染同一条订单记录。
 */
export const saveDoctorOrderRecordCache = (records: OrderSummaryItem[]) => {
  doctorCache.save(
    DOCTOR_CACHE_KEYS.orderRecords,
    normalizeOrderRecords(records)
  );
};

/**
 * 把后端刚创建成功返回的订单记录插入本地缓存列表顶部。
 * 如果缓存中已经存在同 id 记录，会用新记录覆盖旧记录。
 */
export const prependDoctorOrderRecordCache = (record: OrderSummaryItem) => {
  const normalizedRecord = {
    ...record,
    id: Number(record.id),
  };
  const cachedRecords = readDoctorOrderRecordCache() ?? [];
  const nextRecords = [
    normalizedRecord,
    ...cachedRecords.filter(
      (item) => Number(item.id) !== Number(normalizedRecord.id)
    ),
  ];

  saveDoctorOrderRecordCache(nextRecords);
  return nextRecords;
};

/**
 * 从本地缓存读取当前选中的完整订单信息。
 * 该缓存只保存一条记录，进入新订单详情时会覆盖旧记录。
 */
export const readDoctorCurrentOrderDetailCache = () =>
  doctorCache.read<OrderDetailItem>(DOCTOR_CACHE_KEYS.currentOrderDetail);

/**
 * 写入当前选中的完整订单信息，并覆盖上一条详情缓存。
 */
export const saveDoctorCurrentOrderDetailCache = (detail: OrderDetailItem) => {
  doctorCache.save(DOCTOR_CACHE_KEYS.currentOrderDetail, detail);
};

/**
 * 清空医生端全部本地缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧业务数据。
 */
export const clearDoctorDataCache = () => {
  doctorCache.clearAll(Object.values(DOCTOR_CACHE_KEYS));
};
