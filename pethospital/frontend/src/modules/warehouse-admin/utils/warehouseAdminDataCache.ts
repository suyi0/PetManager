import {
  WarehouseItem,
  WarehouseLogItem,
} from "@/modules/warehouse-admin/api/types";
import { createVersionedLocalCacheAccessors } from "@/shared/utils/versionedLocalCache";

const WAREHOUSE_ADMIN_CACHE_OPTIONS = {
  version: 1,
  ttlMs: 1000 * 60 * 60 * 24 * 30,
};

const WAREHOUSE_ADMIN_CACHE_KEYS = {
  items: "warehouse-admin:items:cache",
  operationLogs: "warehouse-admin:operation-logs:cache",
};

const warehouseAdminCache = createVersionedLocalCacheAccessors(
  WAREHOUSE_ADMIN_CACHE_OPTIONS
);

/**
 * 从本地缓存读取仓库端库存列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readWarehouseItemsCache = () =>
  warehouseAdminCache.readArray<WarehouseItem>(
    WAREHOUSE_ADMIN_CACHE_KEYS.items
  );

/**
 * 写入仓库端库存列表本地缓存。
 */
export const saveWarehouseItemsCache = (items: WarehouseItem[]) => {
  warehouseAdminCache.save(WAREHOUSE_ADMIN_CACHE_KEYS.items, items);
};

/**
 * 从本地缓存读取仓库端操作流。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readWarehouseOperationLogsCache = () =>
  warehouseAdminCache.readArray<WarehouseLogItem>(
    WAREHOUSE_ADMIN_CACHE_KEYS.operationLogs
  );

/**
 * 写入仓库端操作流本地缓存。
 */
export const saveWarehouseOperationLogsCache = (logs: WarehouseLogItem[]) => {
  warehouseAdminCache.save(WAREHOUSE_ADMIN_CACHE_KEYS.operationLogs, logs);
};

/**
 * 清空仓库端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧仓库数据。
 */
export const clearWarehouseAdminDataCache = () => {
  warehouseAdminCache.clearAll(Object.values(WAREHOUSE_ADMIN_CACHE_KEYS));
};
