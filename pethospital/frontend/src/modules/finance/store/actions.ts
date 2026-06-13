import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { financeApi } from "../api/financeApi";
import { ChangeSalaryPayload, FinanceHomeData } from "../api/types";
import {
  readFinanceHomeDataCache,
  readFinanceSalaryManagementCache,
  saveFinanceHomeDataCache,
  saveFinanceSalaryManagementCache,
} from "../utils/financeDataCache";
import { FinanceState } from "./types";

type FinanceActionContext = ActionContext<FinanceState, State>;

export const financeActions: ActionTree<FinanceState, State> = {
  /**
   * 确保财务首页实时统计数据可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，WebSocket 推送会持续覆盖这份数据。
   */
  async ensureHomeData(
    { state, commit }: FinanceActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.homeDataMeta, options?.force)) {
      return state.homeData;
    }

    commit("setHomeDataLoading", true);
    try {
      if (!options?.force) {
        const cachedHomeData = readFinanceHomeDataCache();

        if (cachedHomeData) {
          commit("setHomeData", cachedHomeData);
          return cachedHomeData;
        }
      }

      const homeData = await financeApi.getHomeData();
      saveFinanceHomeDataCache(homeData);
      commit("setHomeData", homeData);
      return homeData;
    } finally {
      commit("setHomeDataLoading", false);
    }
  },

  /**
   * 应用财务首页实时推送数据。
   * 首页摘要立即同步到 Vuex 和 localStorage，工资列表只标记为脏数据，
   * 等进入列表或手动刷新时再请求完整工资数据。
   */
  applyRealtimeHomeData(
    { commit }: FinanceActionContext,
    homeData: FinanceHomeData
  ) {
    saveFinanceHomeDataCache(homeData);
    commit("setHomeData", homeData);
    commit("markSalaryManagementDirty");
    return homeData;
  },

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
   * 修改员工工资结构。
   * 工资变动会影响工资管理列表和首页财务摘要，因此成功后统一标脏并刷新。
   */
  async changeSalary(
    { commit, dispatch }: FinanceActionContext,
    payload: ChangeSalaryPayload
  ) {
    await financeApi.changeSalary(payload);
    commit("markSalaryManagementDirty");
    commit("markHomeDataDirty");

    await Promise.all([
      dispatch("ensureSalaryManagement", { force: true }),
      dispatch("ensureHomeData", { force: true }),
    ]);
  },

  markHomeDataDirty({ commit }: FinanceActionContext) {
    commit("markHomeDataDirty");
  },

  markSalaryManagementDirty({ commit }: FinanceActionContext) {
    commit("markSalaryManagementDirty");
  },
};
