import { WarehouseItem, WarehouseLogItem, WarehouseWarningItem } from "./types";

export const warehouseItemsMock: WarehouseItem[] = [
  {
    id: 2058,
    item_name: "犬用营养膏",
    item_type: "营养品",
    item_productiondate: "2025-03-10",
    item_expirationdate: "2026-03-21",
    days_until_expire: 5,
    item_price: 46,
    item_number: 18,
    item_totalprice: 828,
    created_at: "2026-03-16 08:20:00",
    updated_at: "2026-03-16 11:45:00",
  },
  {
    id: 2064,
    item_name: "注射器套装",
    item_type: "耗材",
    item_productiondate: "2025-04-01",
    item_expirationdate: "2027-01-09",
    days_until_expire: 300,
    item_price: 12.5,
    item_number: 96,
    item_totalprice: 1200,
    created_at: "2026-03-16 09:10:00",
    updated_at: "2026-03-16 09:10:00",
  },
  {
    id: 2072,
    item_name: "麻醉针剂",
    item_type: "药品",
    item_productiondate: "2025-06-03",
    item_expirationdate: "2026-09-08",
    days_until_expire: 176,
    item_price: 122,
    item_number: 6,
    item_totalprice: 732,
    created_at: "2026-03-16 10:30:00",
    updated_at: "2026-03-16 10:30:00",
  },
];

export const warehouseWarningsMock: WarehouseWarningItem[] = [
  {
    title: "犬用营养膏",
    description: "临期 5 天",
    level: "warning",
  },
  {
    title: "麻醉针剂",
    description: "库存仅 6",
    level: "danger",
  },
  {
    title: "兔粮补充剂",
    description: "单价上升 32%",
    level: "normal",
  },
];

export const warehouseLogsMock: WarehouseLogItem[] = [
  {
    time: "09:20",
    title: "新增物品 · 犬用营养膏",
    description: "管理员 A-03 完成入库，数量 18，总价自动生成 ¥828.00。",
    tag: "Create",
  },
  {
    time: "11:45",
    title: "更新库存 · 麻醉针剂",
    description: "库存调整为 6，系统同步标记为低库存状态。",
    tag: "Update",
  },
  {
    time: "15:30",
    title: "删除物品 · 旧批次注射器",
    description: "执行删除确认后，记录已从库存面板移除并写入日志。",
    tag: "Delete",
  },
];
