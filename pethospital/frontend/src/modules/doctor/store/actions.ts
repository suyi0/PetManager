import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { doctorApi } from "../api/doctorApi";
import { DoctorState } from "./types";
import {
  readDoctorCurrentUserProfileCache,
  readDoctorCurrentReservationDetailCache,
  readDoctorCurrentOrderDetailCache,
  readDoctorDutyStatusCache,
  readDoctorOrderRecordCache,
  readDoctorQueueItemsCache,
  readDoctorReservationsCache,
  saveDoctorCurrentUserProfileCache,
  saveDoctorCurrentReservationDetailCache,
  saveDoctorCurrentOrderDetailCache,
  saveDoctorDutyStatusCache,
  saveDoctorOrderRecordCache,
  saveDoctorQueueItemsCache,
  saveDoctorReservationsCache,
} from "../utils/doctorDataCache";
import {
  CreateOrderRecordPayload,
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderDetailItem,
  OrderSummaryItem,
  ReservationItem,
} from "../api/types";

type DoctorActionContext = ActionContext<DoctorState, State>;

export const doctorActions: ActionTree<DoctorState, State> = {
  /**
   * 确保医生值班状态可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedStatus = readDoctorDutyStatusCache();

        if (cachedStatus) {
          commit("setDutyStatus", cachedStatus);
          return cachedStatus;
        }
      }

      const dutyStatus = await doctorApi.getDutyStatus();
      saveDoctorDutyStatusCache(dutyStatus);
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
   * 先判断 Vuex 与 localStorage 中缓存的用户编号是否命中；
   * 不命中时从后端获取，并覆盖旧的完整用户详情缓存。
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
      if (!options?.force) {
        const cachedProfile = readDoctorCurrentUserProfileCache();
        if (cachedProfile && Number(cachedProfile.id) === Number(userId)) {
          commit("setCurrentUserProfile", cachedProfile);
          return cachedProfile;
        }
      }

      const profile = await doctorApi.getUserProfiles(userId);
      if (profile) {
        saveDoctorCurrentUserProfileCache(profile);
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
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedQueueItems = readDoctorQueueItemsCache();

        if (cachedQueueItems) {
          commit("setQueueItems", cachedQueueItems);
          return cachedQueueItems;
        }
      }

      const queueItems = await doctorApi.queue();
      saveDoctorQueueItemsCache(queueItems);
      commit("setQueueItems", queueItems);
      return queueItems;
    } finally {
      commit("setQueueItemsLoading", false);
    }
  },

  /**
   * 确保预约列表可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedReservations = readDoctorReservationsCache();

        if (cachedReservations) {
          commit("setReservations", cachedReservations);
          return cachedReservations;
        }
      }

      const reservations = await doctorApi.getReservationsSummary();
      saveDoctorReservationsCache(reservations);
      commit("setReservations", reservations);
      return reservations;
    } finally {
      commit("setReservationsLoading", false);
    }
  },

  /**
   * 按预约编号确保完整预约详情可用。
   * 先判断 Vuex 与 localStorage 中缓存的预约编号是否命中；
   * 不命中时从后端获取，并覆盖旧的完整预约详情缓存。
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
      if (!options?.force) {
        const cachedDetail = readDoctorCurrentReservationDetailCache();
        if (cachedDetail && Number(cachedDetail.id) === Number(reservationId)) {
          commit("setCurrentReservationDetail", cachedDetail);
          return cachedDetail;
        }
      }

      const detail = await doctorApi.getReservationInformation(reservationId);
      if (detail) {
        saveDoctorCurrentReservationDetailCache(detail);
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
   * 更新预约状态，并同步医生端 Vuex 与 localStorage 预约缓存。
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
    saveDoctorReservationsCache(reservations);
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
      saveDoctorCurrentReservationDetailCache(detail);
      commit("setCurrentReservationDetail", detail);
    }
    return reservations;
  },

  /**
   * 创建诊单订单。
   * 后端成功返回订单摘要后，立即写入 Vuex 和 localStorage 列表缓存。
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

    saveDoctorOrderRecordCache(nextRecords);
    commit("setOrderRecords", nextRecords);
    commit("markCurrentOrderDetailDirty");
    return normalizedRecord;
  },

  /**
   * 确保订单记录可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedRecords = readDoctorOrderRecordCache();

        if (cachedRecords) {
          commit("setOrderRecords", cachedRecords);
          return cachedRecords;
        }
      }

      const orderRecords = await doctorApi.getOrderSummary();
      saveDoctorOrderRecordCache(orderRecords);
      commit("setOrderRecords", orderRecords);
      return orderRecords;
    } finally {
      commit("setOrderRecordsLoading", false);
    }
  },

  /**
   * 按订单编号确保完整诊单详情可用。
   * 先判断 Vuex 与 localStorage 中缓存的订单编号是否命中；
   * 不命中时从后端获取，并覆盖旧的完整诊单详情缓存。
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
      if (!options?.force) {
        const cachedDetail = readDoctorCurrentOrderDetailCache();
        if (cachedDetail && Number(cachedDetail.id) === Number(orderId)) {
          commit("setCurrentOrderDetail", cachedDetail);
          return cachedDetail;
        }
      }

      const detail = await doctorApi.getOrderInformation(orderId);
      if (detail) {
        saveDoctorCurrentOrderDetailCache(detail);
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
   * 进入医生端时统一从后端刷新一次核心业务数据，并同步写入本地缓存。
   */
  async ensureWorkbenchData({ dispatch }: DoctorActionContext) {
    await Promise.all([
      dispatch("ensureDutyStatus", { force: true }),
      dispatch("ensureQueueItems", { force: true }),
      dispatch("ensureReservations", { force: true }),
      dispatch("ensureOrderRecords", { force: true }),
    ]);
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
};
