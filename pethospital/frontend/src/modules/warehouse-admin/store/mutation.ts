import { MutationTree } from "vuex";
import {
  WarehouseItem,
  WarehouseLogItem,
} from "@/modules/warehouse-admin/api/types";
import { createWarehouseAdminState } from "./state";
import { WarehouseAdminState } from "./types";

/**
 * 成功拉取数据后统一刷新元信息。
 */
const applyLoadedMeta = (meta: WarehouseAdminState["itemsMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

/**
 * 数据被业务操作影响后，仅标记为脏数据。
 */
const applyDirtyMeta = (meta: WarehouseAdminState["itemsMeta"]) => {
  meta.dirty = true;
};

export const warehouseAdminMutations: MutationTree<WarehouseAdminState> = {
  /**
   * 仓库库存列表加载中状态。
   */
  setItemsLoading(state, loading: boolean) {
    state.itemsMeta.loading = loading;
  },

  /**
   * 写入最新库存列表，并刷新对应元信息。
   */
  setItems(state, items: WarehouseItem[]) {
    state.items = items;
    applyLoadedMeta(state.itemsMeta);
  },

  /**
   * 标记库存列表缓存已过期。
   */
  markItemsDirty(state) {
    applyDirtyMeta(state.itemsMeta);
  },

  /**
   * 操作流加载中状态。
   */
  setLogsLoading(state, loading: boolean) {
    state.logsMeta.loading = loading;
  },

  /**
   * 写入最新操作流，并刷新对应元信息。
   */
  setOperationLogs(state, logs: WarehouseLogItem[]) {
    state.operationLogs = logs;
    applyLoadedMeta(state.logsMeta);
  },

  /**
   * 在当前会话中追加一条新的操作流记录。
   */
  appendOperationLog(state, log: WarehouseLogItem) {
    state.operationLogs = [log, ...state.operationLogs].slice(0, 20);
    applyLoadedMeta(state.logsMeta);
  },

  /**
   * 标记操作流缓存已过期。
   */
  markLogsDirty(state) {
    applyDirtyMeta(state.logsMeta);
  },

  /**
   * 登出时重置整块仓库管理员缓存。
   */
  resetState(state) {
    Object.assign(state, createWarehouseAdminState());
  },
};
