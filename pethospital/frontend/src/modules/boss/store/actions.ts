import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { bossApi } from "../api/bossApi";
import { BossState } from "./types";
import {
  readBossStockDistributionCache,
  saveBossStockDistributionCache,
} from "../utils/bossDataCache";

type BossActionContext = ActionContext<BossState, State>;

export const bossActions: ActionTree<BossState, State> = {
  /**
   * 确保总裁端股权分布数据可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensureStockDistribution(
    { state, commit }: BossActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.stockDistributionMeta, options?.force)) {
      return {
        decisionStocks: state.decisionStocks,
        dividendStocks: state.dividendStocks,
      };
    }

    commit("setStockDistributionLoading", true);
    try {
      if (!options?.force) {
        const cachedDistribution = readBossStockDistributionCache();

        if (cachedDistribution) {
          commit("setStockDistribution", cachedDistribution);
          return cachedDistribution;
        }
      }

      const distribution = await bossApi.getStock();
      saveBossStockDistributionCache(distribution);
      commit("setStockDistribution", distribution);
      return distribution;
    } finally {
      commit("setStockDistributionLoading", false);
    }
  },

  /**
   * 强制刷新总裁端股权分布数据，并同步写入本地缓存。
   */
  async refreshStockDistribution({ dispatch }: BossActionContext) {
    return dispatch("ensureStockDistribution", { force: true });
  },
};
