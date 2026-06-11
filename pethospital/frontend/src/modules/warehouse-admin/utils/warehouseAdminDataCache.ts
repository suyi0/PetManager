import {
  WarehouseItem,
  WarehouseLogItem,
} from "@/modules/warehouse-admin/api/types";
import {
  readVersionedLocalCache,
  saveVersionedLocalCache,
} from "@/shared/utils/versionedLocalCache";

const WAREHOUSE_ADMIN_CACHE_OPTIONS = {
  version: 1,
  ttlMs: 1000 * 60 * 60 * 24 * 30,
};

const WAREHOUSE_ADMIN_CACHE_KEYS = {
  items: "warehouse-admin:items:cache",
  operationLogs: "warehouse-admin:operation-logs:cache",
};

const readJsonCache = <T>(key: string): T | null => {
  return readVersionedLocalCache<T>(key, WAREHOUSE_ADMIN_CACHE_OPTIONS);
};

const saveJsonCache = <T>(key: string, value: T) => {
  saveVersionedLocalCache(key, value, WAREHOUSE_ADMIN_CACHE_OPTIONS);
};

const readArrayCache = <T>(key: string): T[] | null => {
  const cachedValue = readJsonCache<unknown>(key);

  return Array.isArray(cachedValue) ? (cachedValue as T[]) : null;
};

/**
 * 从本地缓存读取仓库端库存列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readWarehouseItemsCache = () =>
  readArrayCache<WarehouseItem>(WAREHOUSE_ADMIN_CACHE_KEYS.items);

/**
 * 写入仓库端库存列表本地缓存。
 */
export const saveWarehouseItemsCache = (items: WarehouseItem[]) => {
  saveJsonCache(WAREHOUSE_ADMIN_CACHE_KEYS.items, items);
};

/**
 * 从本地缓存读取仓库端操作流。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readWarehouseOperationLogsCache = () =>
  readArrayCache<WarehouseLogItem>(WAREHOUSE_ADMIN_CACHE_KEYS.operationLogs);

/**
 * 写入仓库端操作流本地缓存。
 */
export const saveWarehouseOperationLogsCache = (logs: WarehouseLogItem[]) => {
  saveJsonCache(WAREHOUSE_ADMIN_CACHE_KEYS.operationLogs, logs);
};

/**
 * 清空仓库端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧仓库数据。
 */
export const clearWarehouseAdminDataCache = () => {
  Object.values(WAREHOUSE_ADMIN_CACHE_KEYS).forEach((key) => {
    localStorage.removeItem(key);
  });
};
