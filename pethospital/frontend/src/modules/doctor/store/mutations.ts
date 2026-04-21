import { MutationTree } from "vuex";
import {
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderRecordItem,
  QueueItem,
  ReservationItem,
} from "../api/types";
import { createDoctorState } from "./state";
import { DoctorState } from "./types";

const applyLoadedMeta = (meta: DoctorState["dutyStatusMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

const applyDirtyMeta = (meta: DoctorState["dutyStatusMeta"]) => {
  meta.dirty = true;
};

export const doctorMutations: MutationTree<DoctorState> = {
  /**
   * 值班状态加载中。
   */
  setDutyStatusLoading(state, loading: boolean) {
    state.dutyStatusMeta.loading = loading;
  },

  /**
   * 写入最新值班状态。
   */
  setDutyStatus(state, dutyStatus: DoctorDutyStatus) {
    state.dutyStatus = dutyStatus;
    applyLoadedMeta(state.dutyStatusMeta);
  },

  /**
   * 标记值班状态缓存过期。
   */
  markDutyStatusDirty(state) {
    applyDirtyMeta(state.dutyStatusMeta);
  },

  setUserProfilesLoading(state, loading: boolean) {
    state.userProfilesMeta.loading = loading;
  },

  setUserProfiles(state, profiles: DoctorUserProfile[]) {
    state.userProfiles = profiles;
    applyLoadedMeta(state.userProfilesMeta);
  },

  markUserProfilesDirty(state) {
    applyDirtyMeta(state.userProfilesMeta);
  },

  setQueueItemsLoading(state, loading: boolean) {
    state.queueItemsMeta.loading = loading;
  },

  setQueueItems(state, queueItems: QueueItem[]) {
    state.queueItems = queueItems;
    applyLoadedMeta(state.queueItemsMeta);
  },

  markQueueItemsDirty(state) {
    applyDirtyMeta(state.queueItemsMeta);
  },

  setReservationsLoading(state, loading: boolean) {
    state.reservationsMeta.loading = loading;
  },

  setReservations(state, reservations: ReservationItem[]) {
    state.reservations = reservations;
    applyLoadedMeta(state.reservationsMeta);
  },

  markReservationsDirty(state) {
    applyDirtyMeta(state.reservationsMeta);
  },

  setOrderRecordsLoading(state, loading: boolean) {
    state.orderRecordsMeta.loading = loading;
  },

  setOrderRecords(state, orderRecords: OrderRecordItem[]) {
    state.orderRecords = orderRecords;
    applyLoadedMeta(state.orderRecordsMeta);
  },

  markOrderRecordsDirty(state) {
    applyDirtyMeta(state.orderRecordsMeta);
  },

  /**
   * 登出时清空医生端缓存。
   */
  resetState(state) {
    Object.assign(state, createDoctorState());
  },
};
