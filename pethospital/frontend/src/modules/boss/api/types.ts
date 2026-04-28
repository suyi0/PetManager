export type BossStockType = "total" | "remaining" | "holder";

export interface BossStockItem {
  id?: number;
  type: BossStockType;
  holder: string;
  share: number;
  percentage: number;
}

export interface BossStockDistribution {
  decisionStocks: BossStockItem[];
  dividendStocks: BossStockItem[];
}
