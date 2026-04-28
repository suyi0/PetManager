import { ActionContext, ActionTree } from "vuex";
import { State } from "@/store/types";
import { shouldFetch } from "@/store/types";
import { bossApi } from "../api/bossApi";
import { BossState } from "./types";

type BossActionContext = ActionContext<BossState, State>;

export const bossActions: ActionTree<BossState, State> = {
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
      const distribution = await bossApi.getStockDistribution();
      commit("setStockDistribution", distribution);
      return distribution;
    } finally {
      commit("setStockDistributionLoading", false);
    }
  },

  async refreshStockDistribution({ dispatch }: BossActionContext) {
    return dispatch("ensureStockDistribution", { force: true });
  },
};
