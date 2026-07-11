import { MutationTree } from "vuex";
import { createSuperAdminState } from "./state";
import { LogsPayload, SuperAdminState } from "./types";
import {
  AttendanceDevice,
  AttendanceRecord,
  HomePageSummary,
  UserRow,
  WorkTimeRecord,
} from "../api/types";

/**
 * 成功拉取数据后统一更新元信息。
 * 这样各类缓存都能共享同一套“已加载 + 不脏 + 记录时间”的逻辑。
 */
const applyLoadedMeta = (meta: SuperAdminState["usersMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

/**
 * 数据被业务操作影响后，只标记为脏，等待下一次进入页面时再决定是否重拉。
 */
const applyDirtyMeta = (meta: SuperAdminState["usersMeta"]) => {
  meta.dirty = true;
};

export const superAdminMutations: MutationTree<SuperAdminState> = {
  /**
   * 用户列表加载中状态。
   */
  setUsersLoading(state, loading: boolean) {
    state.usersMeta.loading = loading;
  },

  /**
   * 写入最新用户列表，并刷新对应元信息。
   */
  setUsers(state, users: UserRow[]) {
    state.users = users;
    applyLoadedMeta(state.usersMeta);
  },

  /**
   * 标记用户列表缓存已过期。
   */
  markUsersDirty(state) {
    applyDirtyMeta(state.usersMeta);
  },

  /**
   * 考勤列表加载中状态。
   */
  setWorkTimeRecordsLoading(state, loading: boolean) {
    state.workTimeRecordsMeta.loading = loading;
  },

  /**
   * 写入最新考勤记录，并刷新对应元信息。
   */
  setWorkTimeRecords(state, records: WorkTimeRecord[]) {
    state.workTimeRecords = records;
    applyLoadedMeta(state.workTimeRecordsMeta);
  },

  /**
   * 标记考勤列表缓存已过期。
   */
  markWorkTimeRecordsDirty(state) {
    applyDirtyMeta(state.workTimeRecordsMeta);
  },

  setAttendanceRecordsLoading(state, loading: boolean) {
    state.attendanceRecordsMeta.loading = loading;
  },

  setAttendanceRecords(
    state,
    payload: { items: AttendanceRecord[]; total: number }
  ) {
    state.attendanceRecords = payload.items;
    state.attendanceRecordTotal = payload.total;
    applyLoadedMeta(state.attendanceRecordsMeta);
  },

  markAttendanceRecordsDirty(state) {
    applyDirtyMeta(state.attendanceRecordsMeta);
  },

  setAttendanceDevicesLoading(state, loading: boolean) {
    state.attendanceDevicesMeta.loading = loading;
  },

  setAttendanceDevices(state, devices: AttendanceDevice[]) {
    state.attendanceDevices = devices;
    applyLoadedMeta(state.attendanceDevicesMeta);
  },

  markAttendanceDevicesDirty(state) {
    applyDirtyMeta(state.attendanceDevicesMeta);
  },

  /**
   * 日志列表加载中状态。
   */
  setLogsLoading(state, loading: boolean) {
    state.logsMeta.loading = loading;
  },

  /**
   * 写入用户/系统日志，并刷新对应元信息。
   */
  setLogs(state, logs: LogsPayload) {
    state.logs = logs;
    applyLoadedMeta(state.logsMeta);
  },

  /**
   * 标记日志缓存已过期。
   */
  markLogsDirty(state) {
    applyDirtyMeta(state.logsMeta);
  },

  /**
   * 首页摘要加载中状态。
   */
  setHomePageDataLoading(state, loading: boolean) {
    state.homePageDataMeta.loading = loading;
  },

  /**
   * 写入最新首页摘要，并刷新对应元信息。
   */
  setHomePageData(state, summary: HomePageSummary) {
    state.homePageData = summary;
    applyLoadedMeta(state.homePageDataMeta);
  },

  /**
   * 标记首页摘要缓存已过期。
   */
  markHomePageDataDirty(state) {
    applyDirtyMeta(state.homePageDataMeta);
  },

  /**
   * 登出时重置整块超级管理员业务缓存。
   */
  resetState(state) {
    Object.assign(state, createSuperAdminState());
  },
};
