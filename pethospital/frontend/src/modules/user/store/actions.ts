import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { orderApi, petApi, reservationApi } from "../api/userApi";
import { OrderRecordItem } from "../api/types";
import { PetProfile, UserPortalState } from "./types";
import {
  readUserOrderRecordsCache,
  readUserOrderSummariesCache,
  readUserPetProfilesCache,
  readUserReservationDoctorsCache,
  readUserReservationRecordsCache,
  readUserReservationScheduleCache,
  saveUserOrderRecordsCache,
  saveUserOrderSummariesCache,
  saveUserPetProfilesCache,
  saveUserReservationDoctorsCache,
  saveUserReservationRecordsCache,
  saveUserReservationScheduleCache,
} from "../utils/userPortalDataCache";

type UserPortalActionContext = ActionContext<UserPortalState, State>;

const buildUserIdentity = (rootState: State) => ({
  name: rootState.currentUser.userName,
  phone: rootState.currentUser.userPhone,
  email: rootState.currentUser.userEmail,
});

export const userPortalActions: ActionTree<UserPortalState, State> = {
  /**
   * 确保当前用户的宠物档案可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensurePetProfiles(
    { state, commit }: UserPortalActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.petProfilesMeta, options?.force)) {
      return state.petProfiles;
    }

    commit("setPetProfilesLoading", true);
    try {
      if (!options?.force) {
        const cachedPets = readUserPetProfilesCache();

        if (cachedPets) {
          commit("setPetProfiles", cachedPets);
          return cachedPets;
        }
      }

      const pets = await petApi.getPetProfiles();
      saveUserPetProfilesCache(pets);
      commit("setPetProfiles", pets);
      return pets;
    } finally {
      commit("setPetProfilesLoading", false);
    }
  },

  /**
   * 保存当前用户的宠物档案，并同步更新缓存与本地持久化。
   */
  savePetProfiles({ commit }: UserPortalActionContext, pets: PetProfile[]) {
    saveUserPetProfilesCache(pets);
    commit("setPetProfiles", pets);
    return pets;
  },

  /**
   * 保存当前用户的普通订单记录，并同步更新缓存与本地持久化。
   */
  saveOrderRecords(
    { commit }: UserPortalActionContext,
    records: OrderRecordItem[]
  ) {
    saveUserOrderRecordsCache(records);
    commit("setOrderRecords", records);
    return records;
  },

  /**
   * 保存当前用户的预约记录，并同步更新缓存与本地持久化。
   */
  saveReservationRecords(
    { commit }: UserPortalActionContext,
    records: OrderRecordItem[]
  ) {
    saveUserReservationRecordsCache(records);
    commit("setReservationRecords", records);
    return records;
  },

  async createPetProfile(
    { commit, state }: UserPortalActionContext,
    pet: Omit<PetProfile, "id">
  ) {
    const created = await petApi.createPetProfile(pet);
    const pets = [created, ...state.petProfiles];
    saveUserPetProfilesCache(pets);
    commit("setPetProfiles", pets);
    return created;
  },

  async updatePetProfile(
    { commit, state }: UserPortalActionContext,
    pet: PetProfile
  ) {
    const updated = await petApi.updatePetProfile(pet);
    const pets = state.petProfiles.map((item) =>
      item.id === updated.id ? updated : item
    );
    saveUserPetProfilesCache(pets);
    commit("setPetProfiles", pets);
    return updated;
  },

  async deletePetProfile(
    { commit, state }: UserPortalActionContext,
    petId: string
  ) {
    await petApi.deletePetProfile(petId);
    const pets = state.petProfiles.filter((item) => item.id !== petId);
    saveUserPetProfilesCache(pets);
    commit("setPetProfiles", pets);
    return pets;
  },

  /**
   * 确保预约页医生列表可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedDoctors = readUserReservationDoctorsCache();

        if (cachedDoctors) {
          commit("setReservationDoctors", cachedDoctors);
          return cachedDoctors;
        }
      }

      const doctors = await reservationApi.getDoctorOptions();
      saveUserReservationDoctorsCache(doctors);
      commit("setReservationDoctors", doctors);
      return doctors;
    } finally {
      commit("setReservationDoctorsLoading", false);
    }
  },

  /**
   * 确保预约页时间表可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedSchedule = readUserReservationScheduleCache();

        if (cachedSchedule) {
          commit("setReservationSchedule", cachedSchedule);
          return cachedSchedule;
        }
      }

      const schedule = await reservationApi.getScheduleOptions();
      saveUserReservationScheduleCache(schedule);
      commit("setReservationSchedule", schedule);
      return schedule;
    } finally {
      commit("setReservationScheduleLoading", false);
    }
  },

  /**
   * 预约页基础数据预热。
   * 进入用户端时统一从后端刷新预约页常用数据，并同步写入本地缓存。
   */
  async ensureServiceData({ dispatch }: UserPortalActionContext) {
    await Promise.all([
      dispatch("refreshPetProfiles"),
      dispatch("refreshReservationDoctors"),
      dispatch("refreshReservationSchedule"),
    ]);
  },

  /**
   * 确保普通订单记录可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedRecords = readUserOrderRecordsCache();

        if (cachedRecords) {
          commit("setOrderRecords", cachedRecords);
          return cachedRecords;
        }
      }

      const rows = await orderApi.getOrderRecords(buildUserIdentity(rootState));
      saveUserOrderRecordsCache(rows);
      commit("setOrderRecords", rows);
      return rows;
    } finally {
      commit("setOrderRecordsLoading", false);
    }
  },

  /**
   * 确保预约记录可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedRecords = readUserReservationRecordsCache();

        if (cachedRecords) {
          commit("setReservationRecords", cachedRecords);
          return cachedRecords;
        }
      }

      const rows = await reservationApi.getReservationRecords(
        buildUserIdentity(rootState)
      );
      saveUserReservationRecordsCache(rows);
      commit("setReservationRecords", rows);
      return rows;
    } finally {
      commit("setReservationRecordsLoading", false);
    }
  },

  /**
   * 删除一条预约记录，并同步更新本地列表。
   */
  async deleteReservationRecord(
    { state, commit }: UserPortalActionContext,
    reservationId: number
  ) {
    await reservationApi.deleteReservationRecord(reservationId);
    const records = state.reservationRecords.filter(
      (item) => Number(item.id) !== Number(reservationId)
    );
    saveUserReservationRecordsCache(records);
    commit("setReservationRecords", records);
    return records;
  },

  /**
   * 确保订单摘要可用，详情页优先读这里。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedSummaries = readUserOrderSummariesCache();

        if (cachedSummaries) {
          commit("setOrderSummaries", cachedSummaries);
          return cachedSummaries;
        }
      }

      const rows = await orderApi.getOrderSummaries();
      saveUserOrderSummariesCache(rows);
      commit("setOrderSummaries", rows);
      return rows;
    } finally {
      commit("setOrderSummariesLoading", false);
    }
  },

  /**
   * 订单页基础数据预热。
   * 进入订单页时优先复用缓存；需要最新数据时由显式 refresh 动作处理。
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
