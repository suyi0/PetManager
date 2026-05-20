import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { doctorApi } from "../api/doctorApi";
import { DoctorState } from "./types";
import {
  readDoctorDutyStatusCache,
  readDoctorOrderRecordCache,
  readDoctorQueueItemsCache,
  readDoctorReservationsCache,
  readDoctorUserProfilesCache,
  saveDoctorDutyStatusCache,
  saveDoctorOrderRecordCache,
  saveDoctorQueueItemsCache,
  saveDoctorReservationsCache,
  saveDoctorUserProfilesCache,
} from "../utils/doctorDataCache";

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
   * 确保医生端用户档案可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensureUserProfiles(
    { state, commit }: DoctorActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.userProfilesMeta, options?.force)) {
      return state.userProfiles;
    }

    commit("setUserProfilesLoading", true);
    try {
      if (!options?.force) {
        const cachedProfiles = readDoctorUserProfilesCache();

        if (cachedProfiles) {
          commit("setUserProfiles", cachedProfiles);
          return cachedProfiles;
        }
      }

      const profiles = await doctorApi.getUserProfiles();
      saveDoctorUserProfilesCache(profiles);
      commit("setUserProfiles", profiles);
      return profiles;
    } finally {
      commit("setUserProfilesLoading", false);
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

      const queueItems = await doctorApi.getQueueItems();
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

      const reservations = await doctorApi.getReservationsProfiles();
      saveDoctorReservationsCache(reservations);
      commit("setReservations", reservations);
      return reservations;
    } finally {
      commit("setReservationsLoading", false);
    }
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

      const orderRecords = await doctorApi.getOrderRecords();
      saveDoctorOrderRecordCache(orderRecords);
      commit("setOrderRecords", orderRecords);
      return orderRecords;
    } finally {
      commit("setOrderRecordsLoading", false);
    }
  },

  /**
   * 医生端入口数据预热。
   * 进入医生端时统一从后端刷新一次核心业务数据，并同步写入本地缓存。
   */
  async ensureWorkbenchData({ dispatch }: DoctorActionContext) {
    await Promise.all([
      dispatch("refreshDutyStatus"),
      dispatch("refreshQueueItems"),
      dispatch("refreshUserProfiles"),
      dispatch("refreshReservations"),
      dispatch("refreshOrderRecords"),
    ]);
  },

  async refreshDutyStatus({ dispatch }: DoctorActionContext) {
    return dispatch("ensureDutyStatus", { force: true });
  },

  async refreshUserProfiles({ dispatch }: DoctorActionContext) {
    return dispatch("ensureUserProfiles", { force: true });
  },

  async refreshQueueItems({ dispatch }: DoctorActionContext) {
    return dispatch("ensureQueueItems", { force: true });
  },

  async refreshReservations({ dispatch }: DoctorActionContext) {
    return dispatch("ensureReservations", { force: true });
  },

  async refreshOrderRecords({ dispatch }: DoctorActionContext) {
    return dispatch("ensureOrderRecords", { force: true });
  },
};
