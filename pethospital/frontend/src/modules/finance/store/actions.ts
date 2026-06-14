import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { financeApi } from "../api/financeApi";
import { ChangeSalaryPayload, FinanceHomeData } from "../api/types";
import { FinanceState } from "./types";

type FinanceActionContext = ActionContext<FinanceState, State>;

export const financeActions: ActionTree<FinanceState, State> = {
  /**
   * 确保财务首页实时统计数据可用。
   * 首次进入首页时通过 RESTful 获取，后续 WebSocket 推送只更新 Vuex。
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
      const homeData = await financeApi.getHomeData();
      commit("setHomeData", homeData);
      return homeData;
    } finally {
      commit("setHomeDataLoading", false);
    }
  },

  /**
   * 应用财务首页实时推送数据。
   * 首页摘要只同步到 Vuex，工资列表只标记为脏数据，等进入列表或手动刷新时再请求完整工资数据。
   */
  applyRealtimeHomeData(
    { commit }: FinanceActionContext,
    homeData: FinanceHomeData
  ) {
    commit("setHomeData", homeData);
    commit("markSalaryManagementDirty");
    return homeData;
  },

  /**
   * 确保工资管理数据可用。
   * 进入工资管理页时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
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
      const payload = await financeApi.getSalaryManagementData();
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
