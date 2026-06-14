import { MutationTree } from "vuex";
import {
  OrderDetail,
  OrderSummary,
  ReservationOrderRecordItem,
  ReservationSummary,
} from "../api/types";
import { createUserPortalState } from "./state";
import { UserPortalState } from "./types";

/**
 * 处理缓存的元数据。
 * @param meta 元信息对象
 * @remarks
 * 这里的 loaded 和 dirty 字段配合使用，表示数据是否已加载以及是否需要重新加载。
 * - loaded: 数据是否已成功加载过一次。
 * - dirty: 数据是否被标记为过期，需要重新加载。
 * 当数据成功加载后，设置 loaded 为 true，dirty 为 false；当数据被修改或需要刷新时，设置 dirty 为 true。
 */
const applyLoadedMeta = (meta: UserPortalState["reservationRecordsMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

/**
 * 标记数据过期需要重新获取。
 * @param meta 元信息对象
 */
const applyDirtyMeta = (meta: UserPortalState["reservationRecordsMeta"]) => {
  meta.dirty = true;
};

export const userPortalMutations: MutationTree<UserPortalState> = {
  /**
   * 预约记录加载中。
   */
  setReservationRecordsLoading(state, loading: boolean) {
    state.reservationRecordsMeta.loading = loading;
  },

  /**
   * 写入预约记录列表。
   */
  setReservationRecords(state, records: ReservationSummary[]) {
    state.reservationRecords = records;
    applyLoadedMeta(state.reservationRecordsMeta);
  },

  /**
   * 标记预约记录过期。
   */
  markReservationRecordsDirty(state) {
    applyDirtyMeta(state.reservationRecordsMeta);
  },

  /**
   * 当前预约详情加载中。
   */
  setCurrentReservationDetailLoading(state, loading: boolean) {
    state.currentReservationDetailMeta.loading = loading;
  },

  /**
   * 写入当前选中的完整预约详情。
   */
  setCurrentReservationDetail(
    state,
    detail: ReservationOrderRecordItem | null
  ) {
    state.currentReservationDetail = detail;
    applyLoadedMeta(state.currentReservationDetailMeta);
  },

  /**
   * 标记当前预约详情需要重新获取。
   */
  markCurrentReservationDetailDirty(state) {
    applyDirtyMeta(state.currentReservationDetailMeta);
  },

  /**
   * 订单摘要加载中。
   */
  setOrderSummariesLoading(state, loading: boolean) {
    state.orderSummariesMeta.loading = loading;
  },

  /**
   * 写入订单摘要列表，供详情页按编号定位。
   */
  setOrderSummaries(state, rows: OrderSummary[]) {
    state.orderSummaries = rows;
    applyLoadedMeta(state.orderSummariesMeta);
  },

  /**
   * 当前订单详情加载中。
   */
  setCurrentOrderDetailLoading(state, loading: boolean) {
    state.currentOrderDetailMeta.loading = loading;
  },

  /**
   * 写入当前选中的完整订单详情。
   */
  setCurrentOrderDetail(state, detail: OrderDetail | null) {
    state.currentOrderDetail = detail;
    applyLoadedMeta(state.currentOrderDetailMeta);
  },

  /**
   * 标记订单摘要需要重新获取。
   */
  markOrderSummariesDirty(state) {
    applyDirtyMeta(state.orderSummariesMeta);
  },

  /**
   * 标记当前订单详情需要重新获取。
   */
  markCurrentOrderDetailDirty(state) {
    applyDirtyMeta(state.currentOrderDetailMeta);
  },

  /**
   * 登出时清空用户端缓存。
   */
  resetState(state) {
    Object.assign(state, createUserPortalState());
  },
};
