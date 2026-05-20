import { createCacheMeta } from "@/app/store/state";
import { BossState } from "./types";

export const createBossState = (): BossState => ({
  decisionStocks: [],
  dividendStocks: [],
  stockDistributionMeta: createCacheMeta(),
});
