import { CacheMeta } from "@/app/store/types";
import { BossStockItem } from "../api/types";

export interface BossState {
  decisionStocks: BossStockItem[];
  dividendStocks: BossStockItem[];
  stockDistributionMeta: CacheMeta;
}
