import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { bossApi } from "../api/bossApi";
import { BossState } from "./types";

type BossActionContext = ActionContext<BossState, State>;

export const bossActions: ActionTree<BossState, State> = {
  /**
   * 确保总裁端股权分布数据可用。
   * 进入对应页面时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
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
      const distribution = await bossApi.getStock();
      commit("setStockDistribution", distribution);
      return distribution;
    } finally {
      commit("setStockDistributionLoading", false);
    }
  },

  markStockDistributionDirty({ commit }: BossActionContext) {
    commit("markStockDistributionDirty");
  },
};
