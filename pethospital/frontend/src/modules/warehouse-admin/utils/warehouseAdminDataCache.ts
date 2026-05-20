import {
  WarehouseItem,
  WarehouseLogItem,
} from "@/modules/warehouse-admin/api/types";

const WAREHOUSE_ADMIN_CACHE_KEYS = {
  items: "warehouse-admin:items:cache",
  operationLogs: "warehouse-admin:operation-logs:cache",
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
