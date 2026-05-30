import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { orderApi, petApi, reservationApi } from "../api/userApi";
import {
  PetProfile,
  OrderDetail,
  OrderSummary,
  ReservationOrderRecordItem,
  ReservationSummary,
} from "../api/types";
import { UserPortalState } from "./types";
import {
  readUserCurrentReservationDetailCache,
  readUserCurrentOrderDetailCache,
  clearUserCurrentOrderDetailCache,
  clearUserCurrentReservationDetailCache,
  readUserOrderSummariesCache,
  readUserPetProfilesCache,
  readUserReservationDoctorsCache,
  readUserReservationRecordsCache,
  readUserReservationScheduleCache,
  saveUserCurrentReservationDetailCache,
  saveUserCurrentOrderDetailCache,
  saveUserOrderSummariesCache,
  saveUserPetProfilesCache,
  saveUserReservationDoctorsCache,
  saveUserReservationRecordsCache,
  saveUserReservationScheduleCache,
} from "../utils/userPortalDataCache";

type UserPortalActionContext = ActionContext<UserPortalState, State>;

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
   * 保存当前用户的预约记录，并同步更新缓存与本地持久化。
   */
  saveReservationRecords(
    { commit }: UserPortalActionContext,
    records: ReservationSummary[]
  ) {
    saveUserReservationRecordsCache(records);
    commit("setReservationRecords", records);
    return records;
  },

  /**
   * 保存当前用户的订单摘要列表，并同步更新缓存与本地持久化。
   */
  saveOrderSummaries(
    { commit }: UserPortalActionContext,
    summaries: OrderSummary[]
  ) {
    saveUserOrderSummariesCache(summaries);
    commit("setOrderSummaries", summaries);
    return summaries;
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

      const doctors = await reservationApi.getDoctor();
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

      const schedule = await reservationApi.getDate();
      saveUserReservationScheduleCache(schedule);
      commit("setReservationSchedule", schedule);
      return schedule;
    } finally {
      commit("setReservationScheduleLoading", false);
    }
  },

  /**
   * 服务预约页基础数据预热。
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
   * 按订单编号确保完整订单详情可用。
   * 先判断 Vuex 与 localStorage 中缓存的订单编号是否命中；
   * 不命中时从后端获取，并覆盖旧的完整订单详情缓存。
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
      if (!options?.force) {
        const cachedDetail = readUserCurrentOrderDetailCache();
        if (cachedDetail && Number(cachedDetail.id) === Number(orderId)) {
          commit("setCurrentOrderDetail", cachedDetail);
          return cachedDetail;
        }
      }

      const detail = await orderApi.getOrderInformation(orderId);
      if (detail) {
        saveUserCurrentOrderDetailCache(detail);
        commit("setCurrentOrderDetail", detail);
        return detail;
      }

      commit("setCurrentOrderDetail", null);
      clearUserCurrentOrderDetailCache();
      return null;
    } finally {
      commit("setCurrentOrderDetailLoading", false);
    }
  },

  /**
   * 确保预约记录可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedRecords = readUserReservationRecordsCache();

        if (cachedRecords) {
          commit("setReservationRecords", cachedRecords);
          return cachedRecords;
        }
      }

      const rows = await reservationApi.getReservationsSummary();
      saveUserReservationRecordsCache(rows);
      commit("setReservationRecords", rows);
      return rows;
    } finally {
      commit("setReservationRecordsLoading", false);
    }
  },

  /**
   * 按预约编号确保完整预约详情可用。
   * 先判断 Vuex 与 localStorage 中缓存的预约编号是否命中；
   * 不命中时从后端获取，并覆盖旧的完整预约详情缓存。
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
      if (!options?.force) {
        const cachedDetail = readUserCurrentReservationDetailCache();
        if (cachedDetail && Number(cachedDetail.id) === Number(reservationId)) {
          commit("setCurrentReservationDetail", cachedDetail);
          return cachedDetail;
        }
      }

      const detail = await reservationApi.reservationInformation(reservationId);
      if (detail) {
        saveUserCurrentReservationDetailCache(detail);
        commit("setCurrentReservationDetail", detail);
        return detail;
      }

      commit("setCurrentReservationDetail", null);
      clearUserCurrentReservationDetailCache();
      return null;
    } finally {
      commit("setCurrentReservationDetailLoading", false);
    }
  },

  /**
   * 删除一条预约记录，并同步更新本地列表。
   */
  async deleteReservationRecord(
    { state, commit }: UserPortalActionContext,
    reservationId: number
  ) {
    await reservationApi.deleterecord(reservationId);
    const records = state.reservationRecords.filter(
      (item) => Number(item.id) !== Number(reservationId)
    );
    saveUserReservationRecordsCache(records);
    commit("setReservationRecords", records);
    if (
      state.currentReservationDetail &&
      Number(state.currentReservationDetail.id) === Number(reservationId)
    ) {
      commit("setCurrentReservationDetail", null);
      clearUserCurrentReservationDetailCache();
    }
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
    // 这个判断的意思是：如果缓存不为空，并且缓存的更新时间小于30天，则返回缓存的数据；
    // 否则需要重新获取数据。force参数可以强制刷新数据，忽略缓存的有效性。
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

      const rows = await orderApi.getOrderSummary();
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
