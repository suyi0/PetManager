import { MutationTree } from "vuex";
import { DoctorDataItem } from "@/modules/doctor/api/types";
import {
  OrderRecordItem,
  OrderSummary,
  ReservationScheduleState,
} from "../api/types";
import { createUserPortalState } from "./state";
import { PetProfile, UserPortalState } from "./types";

const applyLoadedMeta = (meta: UserPortalState["orderRecordsMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

const applyDirtyMeta = (meta: UserPortalState["orderRecordsMeta"]) => {
  meta.dirty = true;
};

export const userPortalMutations: MutationTree<UserPortalState> = {
  /**
   * 宠物档案加载中。
   */
  setPetProfilesLoading(state, loading: boolean) {
    state.petProfilesMeta.loading = loading;
  },

  /**
   * 写入宠物档案列表。
   */
  setPetProfiles(state, pets: PetProfile[]) {
    state.petProfiles = pets;
    applyLoadedMeta(state.petProfilesMeta);
  },

  /**
   * 标记宠物档案需要重新读取。
   */
  markPetProfilesDirty(state) {
    applyDirtyMeta(state.petProfilesMeta);
  },

  /**
   * 预约医生列表加载中。
   */
  setReservationDoctorsLoading(state, loading: boolean) {
    state.reservationDoctorsMeta.loading = loading;
  },

  /**
   * 写入预约医生列表。
   */
  setReservationDoctors(state, doctors: DoctorDataItem[]) {
    state.reservationDoctors = doctors;
    applyLoadedMeta(state.reservationDoctorsMeta);
  },

  /**
   * 标记预约医生列表需要重新获取。
   */
  markReservationDoctorsDirty(state) {
    applyDirtyMeta(state.reservationDoctorsMeta);
  },

  /**
   * 预约时间表加载中。
   */
  setReservationScheduleLoading(state, loading: boolean) {
    state.reservationScheduleMeta.loading = loading;
  },

  /**
   * 写入预约时间表。
   */
  setReservationSchedule(
    state,
    schedule: Omit<ReservationScheduleState, "doctorData">
  ) {
    state.reservationSchedule = schedule;
    applyLoadedMeta(state.reservationScheduleMeta);
  },

  /**
   * 标记预约时间表需要重新获取。
   */
  markReservationScheduleDirty(state) {
    applyDirtyMeta(state.reservationScheduleMeta);
  },

  /**
   * 订单记录加载中。
   */
  setOrderRecordsLoading(state, loading: boolean) {
    state.orderRecordsMeta.loading = loading;
  },

  /**
   * 写入普通订单记录。
   */
  setOrderRecords(state, records: OrderRecordItem[]) {
    state.orderRecords = records;
    applyLoadedMeta(state.orderRecordsMeta);
  },

  /**
   * 标记普通订单记录过期。
   */
  markOrderRecordsDirty(state) {
    applyDirtyMeta(state.orderRecordsMeta);
  },

  /**
   * 预约记录加载中。
   */
  setReservationRecordsLoading(state, loading: boolean) {
    state.reservationRecordsMeta.loading = loading;
  },

  /**
   * 写入预约记录列表。
   */
  setReservationRecords(state, records: OrderRecordItem[]) {
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
   * 标记订单摘要需要重新获取。
   */
  markOrderSummariesDirty(state) {
    applyDirtyMeta(state.orderSummariesMeta);
  },

  /**
   * 登出时清空用户端缓存。
   */
  resetState(state) {
    Object.assign(state, createUserPortalState());
  },
};
