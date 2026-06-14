import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { doctorApi } from "../api/doctorApi";
import { DoctorState } from "./types";
import {
  CreateOrderRecordPayload,
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderDetailItem,
  OrderSummaryItem,
  ReservationItem,
} from "../api/types";
import {
  DoctorOrderDraft,
  isDoctorOrderDraftExpired,
  listDoctorOrderDrafts,
} from "../utils/orderDrafts";

type DoctorActionContext = ActionContext<DoctorState, State>;

export const doctorActions: ActionTree<DoctorState, State> = {
  /**
   * 确保医生值班状态可用。
   * 默认优先复用当前 Vuex 会话缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensureDutyStatus(
    { state, commit }: DoctorActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.dutyStatusMeta, options?.force)) {
      return state.dutyStatus;
    }

    commit("setDutyStatusLoading", true);
    try {
      const dutyStatus = await doctorApi.getDutyStatus();
      commit("setDutyStatus", dutyStatus);
      return dutyStatus;
    } finally {
      commit("setDutyStatusLoading", false);
    }
  },

  /**
   * 修改医生值班状态。
   * 接口成功后由 store 统一标记并刷新值班状态缓存，组件只负责展示返回结果。
   */
  async changeDutyStatus(
    { commit, dispatch }: DoctorActionContext,
    status: DoctorDutyStatus["status"]
  ) {
    const message = await doctorApi.updateDutyStatus(status);
    commit("markDutyStatusDirty");
    const dutyStatus = await dispatch("ensureDutyStatus", { force: true });

    return {
      message,
      dutyStatus: dutyStatus as DoctorDutyStatus,
    };
  },

  /**
   * 按用户编号确保当前用户详情可用。
   * 只复用当前 Vuex 内存中的详情；刷新页面或首次进入详情页时从后端获取。
   */
  async ensureUserProfile(
    { state, commit }: DoctorActionContext,
    userId: number,
    options?: { force?: boolean }
  ): Promise<DoctorUserProfile | null> {
    if (
      state.currentUserProfile &&
      Number(state.currentUserProfile.id) === Number(userId) &&
      !shouldFetch(state.currentUserProfileMeta, options?.force)
    ) {
      return state.currentUserProfile;
    }

    commit("setCurrentUserProfileLoading", true);
    try {
      const profile = await doctorApi.getUserProfiles(userId);
      if (profile) {
        commit("setCurrentUserProfile", profile);
        return profile;
      }

      commit("setCurrentUserProfile", null);
      return null;
    } finally {
      commit("setCurrentUserProfileLoading", false);
    }
  },

  /**
   * 确保待接诊队列可用。
   * 进入对应页面时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
   */
  async ensureQueueItems(
    { state, commit }: DoctorActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.queueItemsMeta, options?.force)) {
      return state.queueItems;
    }

    commit("setQueueItemsLoading", true);
    try {
      const queueItems = await doctorApi.queue();
      commit("setQueueItems", queueItems);
      return queueItems;
    } finally {
      commit("setQueueItemsLoading", false);
    }
  },

  /**
   * 确保预约列表可用。
   * 进入对应页面时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
   */
  async ensureReservations(
    { state, commit }: DoctorActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.reservationsMeta, options?.force)) {
      return state.reservations;
    }

    commit("setReservationsLoading", true);
    try {
      const reservations = await doctorApi.getReservationsSummary();
      commit("setReservations", reservations);
      return reservations;
    } finally {
      commit("setReservationsLoading", false);
    }
  },

  /**
   * 按预约编号确保完整预约详情可用。
   * 只复用当前 Vuex 内存中的详情；刷新页面或首次进入详情页时从后端获取。
   */
  async ensureReservationDetail(
    { state, commit }: DoctorActionContext,
    reservationId: number,
    options?: { force?: boolean }
  ): Promise<ReservationItem | null> {
    if (
      state.currentReservationDetail &&
      Number(state.currentReservationDetail.id) === Number(reservationId) &&
      !shouldFetch(state.currentReservationDetailMeta, options?.force)
    ) {
      return state.currentReservationDetail;
    }

    commit("setCurrentReservationDetailLoading", true);
    try {
      const detail = await doctorApi.getReservationInformation(reservationId);
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
   * 更新预约状态，并同步医生端 Vuex 内存列表。
   */
  async updateReservationStatus(
    { state, commit }: DoctorActionContext,
    payload: { reservationId: number; status: string }
  ) {
    await doctorApi.updateReservationStatus(
      payload.reservationId,
      payload.status
    );

    const reservations = state.reservations.map((item) =>
      Number(item.id) === Number(payload.reservationId)
        ? { ...item, status: payload.status }
        : item
    );
    commit("setReservations", reservations);
    if (
      state.currentReservationDetail &&
      Number(state.currentReservationDetail.id) ===
        Number(payload.reservationId)
    ) {
      const detail = {
        ...state.currentReservationDetail,
        status: payload.status,
      };
      commit("setCurrentReservationDetail", detail);
    }
    return reservations;
  },

  /**
   * 创建诊单订单。
   * 后端成功返回订单摘要后，立即写入 Vuex 列表缓存。
   */
  async createOrderRecord(
    { state, commit }: DoctorActionContext,
    payload: {
      order: CreateOrderRecordPayload;
      fallback?: Partial<OrderSummaryItem>;
    }
  ) {
    const createdRecord = await doctorApi.createOrderRecord(payload.order);
    const normalizedRecord: OrderSummaryItem = {
      ...createdRecord,
      pet_name:
        createdRecord.pet_name || payload.fallback?.pet_name || "未命名宠物",
      order_type:
        createdRecord.order_type || payload.fallback?.order_type || "诊疗",
      order_data:
        createdRecord.order_data || payload.fallback?.order_data || "",
      order_totalprice:
        createdRecord.order_totalprice ??
        payload.fallback?.order_totalprice ??
        0,
      order_status:
        createdRecord.order_status ||
        payload.fallback?.order_status ||
        "待付款",
    };
    const nextRecords = [
      normalizedRecord,
      ...state.orderRecords.filter(
        (item) => Number(item.id) !== Number(normalizedRecord.id)
      ),
    ];

    commit("setOrderRecords", nextRecords);
    commit("markCurrentOrderDetailDirty");
    return normalizedRecord;
  },

  /**
   * 确保订单记录可用。
   * 进入对应页面时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
   */
  async ensureOrderRecords(
    { state, commit }: DoctorActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.orderRecordsMeta, options?.force)) {
      return state.orderRecords;
    }

    commit("setOrderRecordsLoading", true);
    try {
      const orderRecords = await doctorApi.getOrderSummary();
      commit("setOrderRecords", orderRecords);
      return orderRecords;
    } finally {
      commit("setOrderRecordsLoading", false);
    }
  },

  /**
   * 按订单编号确保完整诊单详情可用。
   * 只复用当前 Vuex 内存中的详情；刷新页面或首次进入详情页时从后端获取。
   */
  async ensureOrderDetail(
    { state, commit }: DoctorActionContext,
    orderId: number,
    options?: { force?: boolean }
  ): Promise<OrderDetailItem | null> {
    if (
      state.currentOrderDetail &&
      Number(state.currentOrderDetail.id) === Number(orderId) &&
      !shouldFetch(state.currentOrderDetailMeta, options?.force)
    ) {
      return state.currentOrderDetail;
    }

    commit("setCurrentOrderDetailLoading", true);
    try {
      const detail = await doctorApi.getOrderInformation(orderId);
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
   * 医生端入口数据预热。
   * 进入医生端时只预热值班状态；列表数据由对应页面进入时通过 RESTful 获取。
   */
  async ensureWorkbenchData({ dispatch }: DoctorActionContext) {
    await dispatch("ensureDutyStatus", { force: true });
  },

  markDutyStatusDirty({ commit }: DoctorActionContext) {
    commit("markDutyStatusDirty");
  },

  markCurrentUserProfileDirty({ commit }: DoctorActionContext) {
    commit("markCurrentUserProfileDirty");
  },

  markQueueItemsDirty({ commit }: DoctorActionContext) {
    commit("markQueueItemsDirty");
  },

  markReservationsDirty({ commit }: DoctorActionContext) {
    commit("markReservationsDirty");
  },

  markCurrentReservationDetailDirty({ commit }: DoctorActionContext) {
    commit("markCurrentReservationDetailDirty");
  },

  markOrderRecordsDirty({ commit }: DoctorActionContext) {
    commit("markOrderRecordsDirty");
  },

  markCurrentOrderDetailDirty({ commit }: DoctorActionContext) {
    commit("markCurrentOrderDetailDirty");
  },

  /**
   * 读取当前会话中的诊单草稿。
   * 草稿超过 24 小时时会自动从 Vuex 内存缓存中移除。
   */
  readOrderDraft(
    { state, commit }: DoctorActionContext,
    draftKey: string
  ): DoctorOrderDraft | null {
    const draft = state.orderDrafts[draftKey];

    if (!draft) {
      return null;
    }

    if (isDoctorOrderDraftExpired(draft)) {
      commit("removeOrderDraft", draftKey);
      return null;
    }

    return draft;
  },

  /**
   * 保存当前会话中的诊单草稿。
   */
  saveOrderDraft(
    { commit }: DoctorActionContext,
    payload: { draftKey: string; draft: DoctorOrderDraft }
  ) {
    commit("setOrderDraft", payload);
    return payload.draft;
  },

  /**
   * 删除当前会话中的指定诊单草稿。
   */
  removeOrderDraft({ commit }: DoctorActionContext, draftKey: string) {
    commit("removeOrderDraft", draftKey);
  },

  /**
   * 获取当前会话中的诊单草稿摘要列表。
   * 过期草稿会在读取时自动清理。
   */
  listOrderDrafts({ state, commit }: DoctorActionContext) {
    Object.entries(state.orderDrafts).forEach(([draftKey, draft]) => {
      if (isDoctorOrderDraftExpired(draft)) {
        commit("removeOrderDraft", draftKey);
      }
    });

    return listDoctorOrderDrafts(state.orderDrafts);
  },
};
