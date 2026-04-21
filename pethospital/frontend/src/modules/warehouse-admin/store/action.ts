import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/store/types";
import { warehouseLogsMock } from "../api/warehouseAdminMock";
import { WarehouseAdminState } from "./types";
import { warehouseAdminApi } from "../api/warehouseAdminApi";

type WarehouseAdminActionContext = ActionContext<WarehouseAdminState, State>;

export const warehouseAdminActions: ActionTree<WarehouseAdminState, State> = {
  /**
   * 确保库存列表可用。
   * 页面首次进入时会请求，之后优先读缓存；只有脏数据、超时或强制刷新时才再次访问接口。
   */
  async ensureItems(
    { state, commit }: WarehouseAdminActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.itemsMeta, options?.force)) {
      return state.items;
    }

    commit("setItemsLoading", true);
    try {
      const items = await warehouseAdminApi.getAllItems();
      commit("setItems", items);
      return items;
    } finally {
      commit("setItemsLoading", false);
    }
  },

  /**
   * 操作流暂时没有后端接口，这里先缓存会话内 mock 数据。
   */
  async ensureLogs(
    { state, commit }: WarehouseAdminActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.logsMeta, options?.force)) {
      return state.operationLogs;
    }

    commit("setLogsLoading", true);
    try {
      commit("setOperationLogs", warehouseLogsMock);
      return warehouseLogsMock;
    } finally {
      commit("setLogsLoading", false);
    }
  },

  /**
   * 仪表盘依赖的基础数据预热。
   */
  async ensureDashboardData({ dispatch }: WarehouseAdminActionContext) {
    await Promise.all([dispatch("ensureItems"), dispatch("ensureLogs")]);
  },

  /**
   * 强制刷新库存列表。
   */
  async refreshItems({ dispatch }: WarehouseAdminActionContext) {
    return dispatch("ensureItems", { force: true });
  },

  /**
   * 强制刷新操作流。
   */
  async refreshLogs({ dispatch }: WarehouseAdminActionContext) {
    return dispatch("ensureLogs", { force: true });
  },
};
