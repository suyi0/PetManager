import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/store/types";
import { orderApi, reservationApi } from "../api/userApi";
import { PetProfile, UserPortalState } from "./types";

type UserPortalActionContext = ActionContext<UserPortalState, State>;

const buildUserIdentity = (rootState: State) => ({
  name: rootState.currentUser.userName,
  phone: rootState.currentUser.userPhone,
  email: rootState.currentUser.userEmail,
});

const getPetStorageKey = (rootState: State) => {
  const userKey =
    rootState.currentUser.userPhone ||
    rootState.currentUser.userEmail ||
    rootState.currentUser.userName ||
    "default-user";

  return `petmanager-user-pets:${userKey}`;
};

const readPetProfiles = (rootState: State): PetProfile[] => {
  if (typeof window === "undefined") {
    return [];
  }

  try {
    const raw = window.localStorage.getItem(getPetStorageKey(rootState));
    const rows = raw ? JSON.parse(raw) : [];
    return Array.isArray(rows) ? (rows as PetProfile[]) : [];
  } catch {
    return [];
  }
};

const persistPetProfiles = (rootState: State, pets: PetProfile[]) => {
  if (typeof window === "undefined") {
    return;
  }

  window.localStorage.setItem(
    getPetStorageKey(rootState),
    JSON.stringify(pets)
  );
};

export const userPortalActions: ActionTree<UserPortalState, State> = {
  /**
   * 确保当前用户的宠物档案可用。
   * 这里的数据本地持久化在 localStorage 中，但展示和编辑统一走 store。
   */
  async ensurePetProfiles(
    { state, commit, rootState }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.petProfilesMeta, options?.force)) {
      return state.petProfiles;
    }

    commit("setPetProfilesLoading", true);
    try {
      const pets = readPetProfiles(rootState);
      commit("setPetProfiles", pets);
      return pets;
    } finally {
      commit("setPetProfilesLoading", false);
    }
  },

  /**
   * 保存当前用户的宠物档案，并同步更新缓存与本地持久化。
   */
  savePetProfiles(
    { commit, rootState }: UserPortalActionContext,
    pets: PetProfile[]
  ) {
    persistPetProfiles(rootState, pets);
    commit("setPetProfiles", pets);
    return pets;
  },

  /**
   * 确保预约页医生列表可用。
   */
  async ensureReservationDoctors(
    { state, commit }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.reservationDoctorsMeta, options?.force)) {
      return state.reservationDoctors;
    }

    commit("setReservationDoctorsLoading", true);
    try {
      const doctors = await reservationApi.getDoctorOptions();
      commit("setReservationDoctors", doctors);
      return doctors;
    } finally {
      commit("setReservationDoctorsLoading", false);
    }
  },

  /**
   * 确保预约页时间表可用。
   */
  async ensureReservationSchedule(
    { state, commit }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.reservationScheduleMeta, options?.force)) {
      return state.reservationSchedule;
    }

    commit("setReservationScheduleLoading", true);
    try {
      const schedule = await reservationApi.getScheduleOptions();
      commit("setReservationSchedule", schedule);
      return schedule;
    } finally {
      commit("setReservationScheduleLoading", false);
    }
  },

  /**
   * 预约页基础数据预热。
   */
  async ensureServiceData({ dispatch }: UserPortalActionContext) {
    await Promise.all([
      dispatch("ensurePetProfiles"),
      dispatch("ensureReservationDoctors"),
      dispatch("ensureReservationSchedule"),
    ]);
  },

  /**
   * 确保普通订单记录可用。
   */
  async ensureOrderRecords(
    { state, commit, rootState }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.orderRecordsMeta, options?.force)) {
      return state.orderRecords;
    }

    commit("setOrderRecordsLoading", true);
    try {
      const rows = await orderApi.getOrderRecords(buildUserIdentity(rootState));
      commit("setOrderRecords", rows);
      return rows;
    } finally {
      commit("setOrderRecordsLoading", false);
    }
  },

  /**
   * 确保预约记录可用。
   */
  async ensureReservationRecords(
    { state, commit, rootState }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.reservationRecordsMeta, options?.force)) {
      return state.reservationRecords;
    }

    commit("setReservationRecordsLoading", true);
    try {
      const rows = await reservationApi.getReservationRecords(
        buildUserIdentity(rootState)
      );
      commit("setReservationRecords", rows);
      return rows;
    } finally {
      commit("setReservationRecordsLoading", false);
    }
  },

  /**
   * 确保订单摘要可用，详情页优先读这里。
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
      const rows = await orderApi.getOrderSummaries();
      commit("setOrderSummaries", rows);
      return rows;
    } finally {
      commit("setOrderSummariesLoading", false);
    }
  },

  /**
   * 订单页基础数据预热。
   */
  async ensureOrderPageData({ dispatch }: UserPortalActionContext) {
    await Promise.all([
      dispatch("ensureOrderRecords"),
      dispatch("ensureReservationRecords"),
      dispatch("ensureOrderSummaries"),
    ]);
  },

  async refreshReservationDoctors({ dispatch }: UserPortalActionContext) {
    return dispatch("ensureReservationDoctors", { force: true });
  },

  async refreshReservationSchedule({ dispatch }: UserPortalActionContext) {
    return dispatch("ensureReservationSchedule", { force: true });
  },

  async refreshOrderRecords({ dispatch }: UserPortalActionContext) {
    return dispatch("ensureOrderRecords", { force: true });
  },

  async refreshReservationRecords({ dispatch }: UserPortalActionContext) {
    return dispatch("ensureReservationRecords", { force: true });
  },

  async refreshOrderSummaries({ dispatch }: UserPortalActionContext) {
    return dispatch("ensureOrderSummaries", { force: true });
  },

  async refreshPetProfiles({ dispatch }: UserPortalActionContext) {
    return dispatch("ensurePetProfiles", { force: true });
  },
};
