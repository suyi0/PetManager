import { CacheMeta } from "@/app/store/types";
import {
  WarehouseItem,
  WarehouseLogItem,
} from "@/modules/warehouse-admin/api/types";

/**
 * 仓库仪表盘顶部卡片的摘要数据。
 * 这部分由库存列表派生出来，不需要额外请求独立接口。
 */
export interface WarehouseDashboardSummary {
  itemCount: number;
  totalValue: number;
  warningCount: number;
}

/**
 * 仓库管理员模块的前端缓存状态。
 * 业务数据只保存在内存中，关闭页面后会自然清空。
 */
export interface WarehouseAdminState {
  items: WarehouseItem[];
  operationLogs: WarehouseLogItem[];
  itemsMeta: CacheMeta;
  logsMeta: CacheMeta;
}
