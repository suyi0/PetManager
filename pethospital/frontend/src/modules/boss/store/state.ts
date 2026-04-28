import { createCacheMeta } from "@/store/state";
import { BossState } from "./types";

export const createBossState = (): BossState => ({
  decisionStocks: [],
  dividendStocks: [],
  stockDistributionMeta: createCacheMeta(),
});
