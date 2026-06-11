import { createCacheMeta } from "@/app/store/state";
import { WarehouseAdminState } from "./types";

/**
 * 创建仓库管理员模块的初始缓存状态。
 */
export const createWarehouseAdminState = (): WarehouseAdminState => ({
  items: [],
  operationLogs: [],
  itemsMeta: createCacheMeta(),
  logsMeta: createCacheMeta(),
});
