import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { financeApi } from "../api/financeApi";
import {
  readFinanceSalaryManagementCache,
  saveFinanceSalaryManagementCache,
} from "../utils/financeDataCache";
import { FinanceState } from "./types";

type FinanceActionContext = ActionContext<FinanceState, State>;

export const financeActions: ActionTree<FinanceState, State> = {
  /**
   * 确保工资管理数据可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensureSalaryManagement(
    { state, commit }: FinanceActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.salaryManagementMeta, options?.force)) {
      return state.salaryManagement;
    }

    commit("setSalaryManagementLoading", true);
    try {
      if (!options?.force) {
        const cachedPayload = readFinanceSalaryManagementCache();

        if (cachedPayload) {
          commit("setSalaryManagement", cachedPayload);
          return cachedPayload;
        }
      }

      const payload = await financeApi.getSalaryManagementData();
      saveFinanceSalaryManagementCache(payload);
      commit("setSalaryManagement", payload);
      return payload;
    } finally {
      commit("setSalaryManagementLoading", false);
    }
  },

  /**
   * 强制刷新工资管理数据。
   */
  async refreshSalaryManagement({ dispatch }: FinanceActionContext) {
    return dispatch("ensureSalaryManagement", { force: true });
  },
};
