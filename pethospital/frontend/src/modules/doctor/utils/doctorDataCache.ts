import {
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderRecordItem,
  QueueItem,
  ReservationItem,
} from "../api/types";

const DOCTOR_CACHE_KEYS = {
  dutyStatus: "doctor:duty-status:cache",
  userProfiles: "doctor:user-profiles:cache",
  queueItems: "doctor:queue-items:cache",
  reservations: "doctor:reservations:cache",
  orderRecords: "doctor:order-records:cache",
};

/**
 * 读取JSON缓存，如果缓存不存在或格式异常则返回 null，并清理损坏的缓存数据。
 * @param key 缓存键
 * @returns 返回解析后的数据对象，如果缓存不存在或格式异常则返回 null
 */
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

/**
 * 处理缓存数据写入，确保数据以 JSON 格式存储，并且在读取时能正确解析。
 * @param key 缓存键
 * @param value 要存储的值
 */
const saveJsonCache = <T>(key: string, value: T) => {
  localStorage.setItem(key, JSON.stringify(value));
};

/**
 * 读取数组类型的本地缓存，确保返回值为数组或 null。
 * @param key 缓存键
 * @returns 数组或 null
 */
const readArrayCache = <T>(key: string): T[] | null => {
  const cachedValue = readJsonCache<unknown>(key);

  return Array.isArray(cachedValue) ? (cachedValue as T[]) : null;
};

/**
 * 标准化订单记录列表，去除重复项。
 * @param records 接收的订单记录列表
 * @returns 标准化后的订单记录列表
 */
const normalizeOrderRecords = (records: OrderRecordItem[]) => {
  const uniqueRecords = new Map<string, OrderRecordItem>();

  records.forEach((record) => {
    uniqueRecords.set(String(record.id), {
      ...record,
      id: String(record.id),
    });
  });

  return [...uniqueRecords.values()];
};

/**
 * 从本地缓存读取医生值班状态。
 * 缓存不存在或格式异常时返回 null，调用方可继续请求后端。
 */
export const readDoctorDutyStatusCache = () =>
  readJsonCache<DoctorDutyStatus>(DOCTOR_CACHE_KEYS.dutyStatus);

/**
 * 写入医生值班状态本地缓存。
 */
export const saveDoctorDutyStatusCache = (status: DoctorDutyStatus) => {
  saveJsonCache(DOCTOR_CACHE_KEYS.dutyStatus, status);
};

/**
 * 从本地缓存读取医生端用户档案列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readDoctorUserProfilesCache = () =>
  readArrayCache<DoctorUserProfile>(DOCTOR_CACHE_KEYS.userProfiles);

/**
 * 写入医生端用户档案列表本地缓存。
 */
export const saveDoctorUserProfilesCache = (profiles: DoctorUserProfile[]) => {
  saveJsonCache(DOCTOR_CACHE_KEYS.userProfiles, profiles);
};

/**
 * 从本地缓存读取待接诊队列列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readDoctorQueueItemsCache = () =>
  readArrayCache<QueueItem>(DOCTOR_CACHE_KEYS.queueItems);

/**
 * 写入待接诊队列列表本地缓存。
 */
export const saveDoctorQueueItemsCache = (queueItems: QueueItem[]) => {
  saveJsonCache(DOCTOR_CACHE_KEYS.queueItems, queueItems);
};

/**
 * 从本地缓存读取医生端预约列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readDoctorReservationsCache = () =>
  readArrayCache<ReservationItem>(DOCTOR_CACHE_KEYS.reservations);

/**
 * 写入医生端预约列表本地缓存。
 */
export const saveDoctorReservationsCache = (
  reservations: ReservationItem[]
) => {
  saveJsonCache(DOCTOR_CACHE_KEYS.reservations, reservations);
};

/**
 * 从本地缓存读取医生端订单记录列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readDoctorOrderRecordCache = () =>
  readArrayCache<OrderRecordItem>(DOCTOR_CACHE_KEYS.orderRecords);

/**
 * 将后端返回的医生端订单记录列表写入浏览器本地缓存。
 * 写入前会按订单 id 去重，避免重复渲染同一条订单记录。
 */
export const saveDoctorOrderRecordCache = (records: OrderRecordItem[]) => {
  saveJsonCache(DOCTOR_CACHE_KEYS.orderRecords, normalizeOrderRecords(records));
};

/**
 * 把后端刚创建成功返回的订单记录插入本地缓存列表顶部。
 * 如果缓存中已经存在同 id 记录，会用新记录覆盖旧记录。
 */
export const prependDoctorOrderRecordCache = (record: OrderRecordItem) => {
  const normalizedRecord = {
    ...record,
    id: String(record.id),
  };
  const cachedRecords = readDoctorOrderRecordCache() ?? [];
  const nextRecords = [
    normalizedRecord,
    ...cachedRecords.filter((item) => item.id !== normalizedRecord.id),
  ];

  saveDoctorOrderRecordCache(nextRecords);
  return nextRecords;
};

/**
 * 清空医生端全部本地缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧业务数据。
 */
export const clearDoctorDataCache = () => {
  Object.values(DOCTOR_CACHE_KEYS).forEach((key) => {
    localStorage.removeItem(key);
  });
};
