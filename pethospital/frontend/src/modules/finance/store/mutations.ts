import { MutationTree } from "vuex";
import { SalaryManagementPayload } from "../api/types";
import { createFinanceState } from "./state";
import { FinanceState } from "./types";

const applyLoadedMeta = (meta: FinanceState["salaryManagementMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

const applyDirtyMeta = (meta: FinanceState["salaryManagementMeta"]) => {
  meta.dirty = true;
};

export const financeMutations: MutationTree<FinanceState> = {
  setSalaryManagementLoading(state, loading: boolean) {
    state.salaryManagementMeta.loading = loading;
  },

  setSalaryManagement(state, payload: SalaryManagementPayload) {
    state.salaryManagement = payload;
    applyLoadedMeta(state.salaryManagementMeta);
  },

  markSalaryManagementDirty(state) {
    applyDirtyMeta(state.salaryManagementMeta);
  },

  resetState(state) {
    Object.assign(state, createFinanceState());
  },
};
