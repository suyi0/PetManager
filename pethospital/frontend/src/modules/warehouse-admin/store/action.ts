import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { WarehouseAdminState } from "./types";
import { warehouseAdminApi } from "@/modules/warehouse-admin/api/warehouseAdminApi";
import {
  WarehouseCreatePayload,
  WarehouseItem,
} from "@/modules/warehouse-admin/api/types";
import {
  readWarehouseItemsCache,
  readWarehouseOperationLogsCache,
  saveWarehouseItemsCache,
  saveWarehouseOperationLogsCache,
} from "@/modules/warehouse-admin/utils/warehouseAdminDataCache";

type WarehouseAdminActionContext = ActionContext<WarehouseAdminState, State>;

const nowTime = () => new Date().toTimeString().slice(0, 5);

const buildLog = (title: string, description: string, tag: string) => ({
  time: nowTime(),
  title,
  description,
  tag,
});

const createUpdatePayload = (
  item: WarehouseItem,
  patch: Partial<WarehouseCreatePayload>
): WarehouseCreatePayload => ({
  id: item.id,
  item_name: patch.item_name ?? item.item_name,
  item_type: patch.item_type ?? item.item_type,
  item_productiondate: patch.item_productiondate ?? item.item_productiondate,
  item_expirationdate: patch.item_expirationdate ?? item.item_expirationdate,
  item_price: Number(patch.item_price ?? item.item_price),
  item_number: Number(patch.item_number ?? item.item_number),
});

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

      const items = await warehouseAdminApi.select();
      saveWarehouseItemsCache(items);
      commit("setItems", items);
      return items;
    } finally {
      commit("setItemsLoading", false);
    }
  },

  /**
   * 确保操作流可用。
   * 操作流暂时没有后端接口，因此只复用 Vuex 和 localStorage 缓存。
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

      commit("setOperationLogs", []);
      return [];
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

  /**
   * 新增物品入库；成功后刷新库存并写入本地缓存。
   */
  async createItem(
    { commit, dispatch, state }: WarehouseAdminActionContext,
    payload: WarehouseCreatePayload
  ) {
    await warehouseAdminApi.upload(payload);
    commit("markItemsDirty");
    commit(
      "appendOperationLog",
      buildLog(
        `入库新增 · ${payload.item_name}`,
        `数量 ${payload.item_number}，单价 ¥${Number(
          payload.item_price
        ).toFixed(2)}。`,
        "IN"
      )
    );
    saveWarehouseOperationLogsCache(state.operationLogs);
    await dispatch("refreshItems");
  },

  /**
   * 更新物品资料或库存数量；成功后刷新库存并写入本地缓存。
   */
  async updateItem(
    { state, commit, dispatch }: WarehouseAdminActionContext,
    payload: { itemId: number; patch: Partial<WarehouseCreatePayload> }
  ) {
    const item = state.items.find((row) => row.id === payload.itemId);
    if (!item) {
      throw new Error("未找到要更新的库存物品");
    }

    const nextPayload = createUpdatePayload(item, payload.patch);
    await warehouseAdminApi.updata(payload.itemId, nextPayload);
    commit("markItemsDirty");
    commit(
      "appendOperationLog",
      buildLog(
        `库存更新 · ${nextPayload.item_name}`,
        `当前库存 ${nextPayload.item_number}，单价 ¥${Number(
          nextPayload.item_price
        ).toFixed(2)}。`,
        "EDIT"
      )
    );
    saveWarehouseOperationLogsCache(state.operationLogs);
    await dispatch("refreshItems");
  },

  /**
   * 已有物品入库，表现为增加库存数量。
   */
  async stockIn(
    { state, dispatch }: WarehouseAdminActionContext,
    payload: { itemId: number; quantity: number }
  ) {
    const item = state.items.find((row) => row.id === payload.itemId);
    if (!item) {
      throw new Error("未找到要入库的库存物品");
    }

    if (payload.quantity <= 0) {
      throw new Error("入库数量必须大于 0");
    }

    await dispatch("updateItem", {
      itemId: payload.itemId,
      patch: { item_number: item.item_number + payload.quantity },
    });
  },

  /**
   * 已有物品出库，表现为减少库存数量。
   */
  async stockOut(
    { state, dispatch }: WarehouseAdminActionContext,
    payload: { itemId: number; quantity: number }
  ) {
    const item = state.items.find((row) => row.id === payload.itemId);
    if (!item) {
      throw new Error("未找到要出库的库存物品");
    }

    if (payload.quantity <= 0 || payload.quantity > item.item_number) {
      throw new Error("出库数量必须大于 0 且不能超过当前库存");
    }

    await dispatch("updateItem", {
      itemId: payload.itemId,
      patch: { item_number: item.item_number - payload.quantity },
    });
  },

  /**
   * 软删除库存物品；成功后刷新库存并写入本地缓存。
   */
  async deleteItem(
    { state, commit, dispatch }: WarehouseAdminActionContext,
    itemId: number
  ) {
    const item = state.items.find((row) => row.id === itemId);
    await warehouseAdminApi.delete(itemId);
    commit("markItemsDirty");
    commit(
      "appendOperationLog",
      buildLog(
        `删除物品 · ${item?.item_name ?? itemId}`,
        `ID ${itemId} 已从库存列表中移除。`,
        "DEL"
      )
    );
    saveWarehouseOperationLogsCache(state.operationLogs);
    await dispatch("refreshItems");
  },
};
