import { createCacheMeta } from "@/store/state";
import { warehouseLogsMock } from "../api/warehouseAdminMock";
import { WarehouseAdminState } from "./types";

/**
 * 创建仓库管理员模块的初始缓存状态。
 */
export const createWarehouseAdminState = (): WarehouseAdminState => ({
  items: [],
  operationLogs: warehouseLogsMock,
  itemsMeta: createCacheMeta(),
  logsMeta: createCacheMeta(),
});
