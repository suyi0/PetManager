export interface WarehouseItem {
  id: number;
  item_name: string;
  item_type: string;
  item_productiondate: string;
  item_expirationdate: string;
  days_until_expire: number | null;
  item_price: number;
  item_number: number;
  item_totalprice: number;
  created_at: string;
  updated_at: string;
}

export interface WarehouseCreatePayload {
  id?: number;
  item_name: string;
  item_type: string;
  item_productiondate: string;
  item_expirationdate: string;
  item_price: number;
  item_number: number;
}

export interface WarehouseWarningItem {
  title: string;
  description: string;
  level: "normal" | "warning" | "danger";
}

export interface WarehouseLogItem {
  time: string;
  title: string;
  description: string;
  tag: string;
}

/**
 * 仓库端顶部摘要卡片数据。
 */
export interface WarehouseDashboardSummary {
  itemCount: number;
  totalValue: number;
  warningCount: number;
}

export interface ApiListResponse<T> {
  success?: boolean;
  data?: T[];
  message?: string;
}
