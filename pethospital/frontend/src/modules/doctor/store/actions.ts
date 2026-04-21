import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/store/types";
import { doctorApi } from "../api/doctorApi";
import { DoctorState } from "./types";

type DoctorActionContext = ActionContext<DoctorState, State>;

export const doctorActions: ActionTree<DoctorState, State> = {
  /**
   * 确保医生值班状态可用。
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
   * 确保医生端用户档案可用。
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
      const profiles = await doctorApi.getUserProfiles();
      commit("setUserProfiles", profiles);
      return profiles;
    } finally {
      commit("setUserProfilesLoading", false);
    }
  },

  /**
   * 确保待接诊队列可用。
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
      const queueItems = await doctorApi.getQueueItems();
      commit("setQueueItems", queueItems);
      return queueItems;
    } finally {
      commit("setQueueItemsLoading", false);
    }
  },

  /**
   * 确保预约列表可用。
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
      const reservations = await doctorApi.getReservationsProfiles();
      commit("setReservations", reservations);
      return reservations;
    } finally {
      commit("setReservationsLoading", false);
    }
  },

  /**
   * 确保订单记录可用。
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
      const orderRecords = await doctorApi.getOrderRecords();
      commit("setOrderRecords", orderRecords);
      return orderRecords;
    } finally {
      commit("setOrderRecordsLoading", false);
    }
  },

  /**
   * 工作台依赖的基础数据预热。
   */
  async ensureWorkbenchData({ dispatch }: DoctorActionContext) {
    await Promise.all([
      dispatch("ensureDutyStatus"),
      dispatch("ensureQueueItems"),
      dispatch("ensureUserProfiles"),
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
