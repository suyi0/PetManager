import { MutationTree } from "vuex";
import { BossStockDistribution } from "../api/types";
import { createBossState } from "./state";
import { BossState } from "./types";

const applyLoadedMeta = (meta: BossState["stockDistributionMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

const applyDirtyMeta = (meta: BossState["stockDistributionMeta"]) => {
  meta.dirty = true;
};

export const bossMutations: MutationTree<BossState> = {
  setStockDistributionLoading(state, loading: boolean) {
    state.stockDistributionMeta.loading = loading;
  },

  setStockDistribution(state, payload: BossStockDistribution) {
    state.decisionStocks = payload.decisionStocks;
    state.dividendStocks = payload.dividendStocks;
    applyLoadedMeta(state.stockDistributionMeta);
  },

  markStockDistributionDirty(state) {
    applyDirtyMeta(state.stockDistributionMeta);
  },

  resetState(state) {
    Object.assign(state, createBossState());
  },
};
