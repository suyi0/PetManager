import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { orderApi, reservationApi } from "../api/userApi";
import {
  OrderDetail,
  OrderSummary,
  ReservationOrderRecordItem,
  ReservationSummary,
} from "../api/types";
import { UserPortalState } from "./types";

type UserPortalActionContext = ActionContext<UserPortalState, State>;
type CreateReservationPayload = {
  name?: string | null;
  phone?: string | null;
  email?: string | null;
  doctorId: number;
  petId: number;
  reservationType: string;
  date: string;
  slot: string;
};

export const userPortalActions: ActionTree<UserPortalState, State> = {
  /**
   * 保存当前用户的预约记录，并同步更新 Vuex 内存缓存。
   */
  saveReservationRecords(
    { commit }: UserPortalActionContext,
    records: ReservationSummary[]
  ) {
    commit("setReservationRecords", records);
    return records;
  },

  /**
   * 保存当前用户的订单摘要列表，并同步更新 Vuex 内存缓存。
   */
  saveOrderSummaries(
    { commit }: UserPortalActionContext,
    summaries: OrderSummary[]
  ) {
    commit("setOrderSummaries", summaries);
    return summaries;
  },

  /**
   * 创建预约记录。
   * 当前接口不保证返回完整预约摘要，因此成功后统一标记预约和订单摘要过期。
   */
  async createReservationRecord(
    { commit }: UserPortalActionContext,
    payload: CreateReservationPayload
  ) {
    const response = await reservationApi.record(payload);
    commit("markReservationRecordsDirty");
    commit("markOrderSummariesDirty");
    return response;
  },

  /**
   * 按订单编号确保完整订单详情可用。
   * 只复用当前 Vuex 内存中的详情；刷新页面或首次进入详情页时从后端获取。
   */
  async ensureOrderDetail(
    { state, commit }: UserPortalActionContext,
    orderId: number,
    options?: { force?: boolean }
  ): Promise<OrderDetail | null> {
    if (
      state.currentOrderDetail &&
      Number(state.currentOrderDetail.id) === Number(orderId) &&
      !shouldFetch(state.currentOrderDetailMeta, options?.force)
    ) {
      return state.currentOrderDetail;
    }

    commit("setCurrentOrderDetailLoading", true);
    try {
      const detail = await orderApi.getOrderInformation(orderId);
      if (detail) {
        commit("setCurrentOrderDetail", detail);
        return detail;
      }

      commit("setCurrentOrderDetail", null);
      return null;
    } finally {
      commit("setCurrentOrderDetailLoading", false);
    }
  },

  /**
   * 确保预约记录可用。
   * 用户进入列表页时通过 RESTful 获取一次摘要数据，只复用当前 Vuex 内存缓存。
   */
  async ensureReservationRecords(
    { state, commit }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.reservationRecordsMeta, options?.force)) {
      return state.reservationRecords;
    }

    commit("setReservationRecordsLoading", true);
    try {
      const rows = await reservationApi.getReservationsSummary();
      commit("setReservationRecords", rows);
      return rows;
    } finally {
      commit("setReservationRecordsLoading", false);
    }
  },

  /**
   * 按预约编号确保完整预约详情可用。
   * 只复用当前 Vuex 内存中的详情；刷新页面或首次进入详情页时从后端获取。
   */
  async ensureReservationDetail(
    { state, commit }: UserPortalActionContext,
    reservationId: number,
    options?: { force?: boolean }
  ): Promise<ReservationOrderRecordItem | null> {
    if (
      state.currentReservationDetail &&
      Number(state.currentReservationDetail.id) === Number(reservationId) &&
      !shouldFetch(state.currentReservationDetailMeta, options?.force)
    ) {
      return state.currentReservationDetail;
    }

    commit("setCurrentReservationDetailLoading", true);
    try {
      const detail = await reservationApi.reservationInformation(reservationId);
      if (detail) {
        commit("setCurrentReservationDetail", detail);
        return detail;
      }

      commit("setCurrentReservationDetail", null);
      return null;
    } finally {
      commit("setCurrentReservationDetailLoading", false);
    }
  },

  /**
   * 删除一条预约记录，并同步更新当前 Vuex 列表。
   */
  async deleteReservationRecord(
    { state, commit }: UserPortalActionContext,
    reservationId: number
  ) {
    await reservationApi.deleterecord(reservationId);
    const records = state.reservationRecords.filter(
      (item) => Number(item.id) !== Number(reservationId)
    );
    commit("setReservationRecords", records);
    if (
      state.currentReservationDetail &&
      Number(state.currentReservationDetail.id) === Number(reservationId)
    ) {
      commit("setCurrentReservationDetail", null);
    }
    return records;
  },

  /**
   * 确保订单摘要可用。
   * 用户进入列表页时通过 RESTful 获取一次摘要数据，只复用当前 Vuex 内存缓存。
   */
  async ensureOrderSummaries(
    { state, commit }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.orderSummariesMeta, options?.force)) {
      return state.orderSummaries;
    }

    commit("setOrderSummariesLoading", true);
    try {
      const rows = await orderApi.getOrderSummary();
      commit("setOrderSummaries", rows);
      return rows;
    } finally {
      commit("setOrderSummariesLoading", false);
    }
  },

  /**
   * 订单页基础数据预热。
   * 进入订单页时通过 RESTful 获取预约和订单摘要；同一次页面会话内复用 Vuex。
   */
  async ensureOrderPageData(
    { dispatch }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    await Promise.all([
      dispatch("ensureReservationRecords", options),
      dispatch("ensureOrderSummaries", options),
    ]);
  },

  markReservationRecordsDirty({ commit }: UserPortalActionContext) {
    commit("markReservationRecordsDirty");
  },

  markOrderSummariesDirty({ commit }: UserPortalActionContext) {
    commit("markOrderSummariesDirty");
  },

  markCurrentReservationDetailDirty({ commit }: UserPortalActionContext) {
    commit("markCurrentReservationDetailDirty");
  },

  markCurrentOrderDetailDirty({ commit }: UserPortalActionContext) {
    commit("markCurrentOrderDetailDirty");
  },
};
