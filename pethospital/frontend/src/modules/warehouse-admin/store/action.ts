import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { warehouseLogsMock } from "@/modules/warehouse-admin/api/warehouseAdminMock";
import { WarehouseAdminState } from "./types";
import { warehouseAdminApi } from "@/modules/warehouse-admin/api/warehouseAdminApi";
import {
  readWarehouseItemsCache,
  readWarehouseOperationLogsCache,
  saveWarehouseItemsCache,
  saveWarehouseOperationLogsCache,
} from "@/modules/warehouse-admin/utils/warehouseAdminDataCache";

type WarehouseAdminActionContext = ActionContext<WarehouseAdminState, State>;

export const warehouseAdminActions: ActionTree<WarehouseAdminState, State> = {
  /**
   * 确保库存列表可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedItems = readWarehouseItemsCache();

        if (cachedItems) {
          commit("setItems", cachedItems);
          return cachedItems;
        }
      }

      const items = await warehouseAdminApi.getAllItems();
      saveWarehouseItemsCache(items);
      commit("setItems", items);
      return items;
    } finally {
      commit("setItemsLoading", false);
    }
  },

  /**
   * 确保操作流可用。
   * 操作流暂时没有后端接口，因此默认优先复用 Vuex 和 localStorage 缓存。
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
      if (!options?.force) {
        const cachedLogs = readWarehouseOperationLogsCache();

        if (cachedLogs) {
          commit("setOperationLogs", cachedLogs);
          return cachedLogs;
        }
      }

      saveWarehouseOperationLogsCache(warehouseLogsMock);
      commit("setOperationLogs", warehouseLogsMock);
      return warehouseLogsMock;
    } finally {
      commit("setLogsLoading", false);
    }
  },

  /**
   * 仪表盘依赖的基础数据预热。
   * 进入仓库端时统一从后端刷新核心库存数据，并同步写入本地缓存。
   */
  async ensureDashboardData({ dispatch }: WarehouseAdminActionContext) {
    await Promise.all([dispatch("refreshItems"), dispatch("refreshLogs")]);
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
