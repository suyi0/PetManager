import { MutationTree } from "vuex";
import {
  DoctorDutyStatus,
  DoctorUserProfile,
  OrderDetailItem,
  OrderRecordItem,
  QueueItem,
  ReservationItem,
  ReservationSummaryItem,
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

  /**
   * 用户档案加载中。
   * @param state 医生状态对象
   * @param loading 是否正在加载用户档案
   */
  setUserProfilesLoading(state, loading: boolean) {
    state.userProfilesMeta.loading = loading;
  },

  /**
   * 写入最新用户档案。
   * @param state 医生状态对象
   * @param profiles 用户档案列表，每个档案包含用户的基本信息、宠物档案列表和订单详情列表等数据
   */
  setUserProfiles(state, profiles: DoctorUserProfile[]) {
    state.userProfiles = profiles;
    applyLoadedMeta(state.userProfilesMeta);
  },

  /**
   * 标记用户档案缓存过期。
   * @param state 医生状态对象
   */
  markUserProfilesDirty(state) {
    applyDirtyMeta(state.userProfilesMeta);
  },

  /**
   * 队列项目加载中。
   * @param state 医生状态对象
   * @param loading 是否正在加载队列项目
   */
  setQueueItemsLoading(state, loading: boolean) {
    state.queueItemsMeta.loading = loading;
  },

  /**
   * 写入最新队列项目。
   * @param state 医生状态对象
   * @param queueItems 队列项目列表，每个项目包含宠物的基本信息、症状描述和到院时间等数据
   */
  setQueueItems(state, queueItems: QueueItem[]) {
    state.queueItems = queueItems;
    applyLoadedMeta(state.queueItemsMeta);
  },

  /**
   * 标记队列项目缓存过期。
   * @param state 医生状态对象
   */
  markQueueItemsDirty(state) {
    applyDirtyMeta(state.queueItemsMeta);
  },

  /**
   * 预约项目加载中。
   * @param state 医生状态对象
   * @param loading 是否正在加载预约项目
   */
  setReservationsLoading(state, loading: boolean) {
    state.reservationsMeta.loading = loading;
  },

  /**
   * 写入最新预约项目。
   * @param state 医生状态对象
   * @param reservations 预约项目列表，每个项目包含预约的基本信息、宠物信息和医生信息等数据
   */
  setReservations(state, reservations: ReservationSummaryItem[]) {
    state.reservations = reservations;
    applyLoadedMeta(state.reservationsMeta);
  },

  /**
   * 标记预约项目缓存过期。
   * @param state 医生状态对象
   */
  markReservationsDirty(state) {
    applyDirtyMeta(state.reservationsMeta);
  },

  /**
   * 预约详情加载中。
   */
  setCurrentReservationDetailLoading(state, loading: boolean) {
    state.currentReservationDetailMeta.loading = loading;
  },

  /**
   * 写入当前选中的完整预约详情。
   */
  setCurrentReservationDetail(state, detail: ReservationItem | null) {
    state.currentReservationDetail = detail;
    applyLoadedMeta(state.currentReservationDetailMeta);
  },

  /**
   * 标记当前预约详情缓存过期。
   */
  markCurrentReservationDetailDirty(state) {
    applyDirtyMeta(state.currentReservationDetailMeta);
  },

  /**
   * 订单记录加载中。
   * @param state 医生状态对象
   * @param loading 是否正在加载订单记录
   */
  setOrderRecordsLoading(state, loading: boolean) {
    state.orderRecordsMeta.loading = loading;
  },

  /**
   * 最新订单记录写入Vuex缓存。
   * @param state 医生状态对象
   * @param orderRecords 订单记录列表，每个记录包含订单的基本信息、宠物信息和医生信息等数据
   */
  setOrderRecords(state, orderRecords: OrderRecordItem[]) {
    state.orderRecords = orderRecords;
    applyLoadedMeta(state.orderRecordsMeta);
  },

  /**
   * 标记订单记录缓存过期。
   * @param state 医生状态对象
   */
  markOrderRecordsDirty(state) {
    applyDirtyMeta(state.orderRecordsMeta);
  },

  /**
   * 订单详情加载中。
   */
  setCurrentOrderDetailLoading(state, loading: boolean) {
    state.currentOrderDetailMeta.loading = loading;
  },

  /**
   * 写入当前选中的完整订单详情。
   */
  setCurrentOrderDetail(state, detail: OrderDetailItem | null) {
    state.currentOrderDetail = detail;
    applyLoadedMeta(state.currentOrderDetailMeta);
  },

  /**
   * 标记当前订单详情缓存过期。
   */
  markCurrentOrderDetailDirty(state) {
    applyDirtyMeta(state.currentOrderDetailMeta);
  },

  /**
   * 登出时清空医生端缓存。
   */
  resetState(state) {
    Object.assign(state, createDoctorState());
  },
};
