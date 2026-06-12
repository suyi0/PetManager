import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { superAdminApi } from "../api/superAdminApi";
import { SuperAdminState } from "./types";
import { CreateUserPayload } from "../api/types";
import {
  readSuperAdminHomePageDataCache,
  readSuperAdminLogsCache,
  readSuperAdminUsersCache,
  readSuperAdminWorkTimeRecordsCache,
  saveSuperAdminHomePageDataCache,
  saveSuperAdminLogsCache,
  saveSuperAdminUsersCache,
  saveSuperAdminWorkTimeRecordsCache,
} from "../utils/superAdminDataCache";

type SuperAdminActionContext = ActionContext<SuperAdminState, State>;

export const superAdminActions: ActionTree<SuperAdminState, State> = {
  /**
   * 确保用户列表可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensureUsers(
    { state, commit }: SuperAdminActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.usersMeta, options?.force)) {
      return state.users;
    }

    commit("setUsersLoading", true);
    try {
      if (!options?.force) {
        const cachedUsers = readSuperAdminUsersCache();

        if (cachedUsers) {
          commit("setUsers", cachedUsers);
          return cachedUsers;
        }
      }

      const users = await superAdminApi.getUsers();
      saveSuperAdminUsersCache(users);
      commit("setUsers", users);
      return users;
    } finally {
      commit("setUsersLoading", false);
    }
  },

  /**
   * 确保考勤列表可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensureWorkTimeRecords(
    { state, commit }: SuperAdminActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.workTimeRecordsMeta, options?.force)) {
      return state.workTimeRecords;
    }

    commit("setWorkTimeRecordsLoading", true);
    try {
      if (!options?.force) {
        const cachedRecords = readSuperAdminWorkTimeRecordsCache();

        if (cachedRecords) {
          commit("setWorkTimeRecords", cachedRecords);
          return cachedRecords;
        }
      }

      const records = await superAdminApi.getWorkTimeRecord();
      saveSuperAdminWorkTimeRecordsCache(records);
      commit("setWorkTimeRecords", records);
      return records;
    } finally {
      commit("setWorkTimeRecordsLoading", false);
    }
  },

  /**
   * 确保日志列表可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensureLogs(
    { state, commit }: SuperAdminActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.logsMeta, options?.force)) {
      return state.logs;
    }

    commit("setLogsLoading", true);
    try {
      if (!options?.force) {
        const cachedLogs = readSuperAdminLogsCache();

        if (cachedLogs) {
          commit("setLogs", cachedLogs);
          return cachedLogs;
        }
      }

      const logs = await superAdminApi.getLogs();
      saveSuperAdminLogsCache(logs);
      commit("setLogs", logs);
      return logs;
    } finally {
      commit("setLogsLoading", false);
    }
  },

  /**
   * 确保首页摘要数据可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
   */
  async ensureHomePageData(
    { state, commit }: SuperAdminActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.homePageDataMeta, options?.force)) {
      return state.homePageData;
    }

    commit("setHomePageDataLoading", true);
    try {
      if (!options?.force) {
        const cachedHomePageData = readSuperAdminHomePageDataCache();

        if (cachedHomePageData) {
          commit("setHomePageData", cachedHomePageData);
          return cachedHomePageData;
        }
      }

      const homePageData = await superAdminApi.homePageGetData();
      saveSuperAdminHomePageDataCache(homePageData);
      commit("setHomePageData", homePageData);
      return homePageData;
    } finally {
      commit("setHomePageDataLoading", false);
    }
  },

  /**
   * 确保总览页摘要数据可用。
   */
  async ensureOverviewData({ dispatch }: SuperAdminActionContext) {
    await dispatch("ensureHomePageData");
  },

  /**
   * 在线医生页复用用户列表和考勤记录两份缓存。
   * 只有其中任一缓存首次进入、超时或被标脏时，才会真正重新请求。
   */
  async ensureOnlineDoctorsData({ dispatch }: SuperAdminActionContext) {
    await Promise.all([
      dispatch("ensureUsers"),
      dispatch("ensureWorkTimeRecords"),
    ]);
  },

  /**
   * 超级管理端入口数据预热。
   * 进入超级管理端时统一从后端刷新核心业务数据，并同步写入本地缓存。
   */
  async refreshSuperAdminData({ dispatch }: SuperAdminActionContext) {
    await Promise.all([
      dispatch("refreshUsers"),
      dispatch("refreshWorkTimeRecords"),
      dispatch("refreshLogs"),
      dispatch("refreshHomePageData"),
    ]);
  },

  /**
   * 强制刷新用户列表。
   */
  async refreshUsers({ dispatch }: SuperAdminActionContext) {
    return dispatch("ensureUsers", { force: true });
  },

  /**
   * 强制刷新考勤列表。
   */
  async refreshWorkTimeRecords({ dispatch }: SuperAdminActionContext) {
    return dispatch("ensureWorkTimeRecords", { force: true });
  },

  /**
   * 强制刷新日志列表。
   */
  async refreshLogs({ dispatch }: SuperAdminActionContext) {
    return dispatch("ensureLogs", { force: true });
  },

  /**
   * 强制刷新首页摘要。
   */
  async refreshHomePageData({ dispatch }: SuperAdminActionContext) {
    return dispatch("ensureHomePageData", { force: true });
  },

  /**
   * 强制刷新首页摘要数据。
   */
  async refreshOverviewData({ dispatch }: SuperAdminActionContext) {
    await dispatch("refreshHomePageData");
  },

  /**
   * 在线医生页显式刷新时，同时刷新医生资料与值班记录。
   */
  async refreshOnlineDoctorsData({ dispatch }: SuperAdminActionContext) {
    await Promise.all([
      dispatch("refreshUsers"),
      dispatch("refreshWorkTimeRecords"),
    ]);
  },

  async createUser(
    { commit, dispatch }: SuperAdminActionContext,
    payload: CreateUserPayload
  ) {
    await superAdminApi.createUser(payload);
    commit("markUsersDirty");
    commit("markLogsDirty");
    commit("markHomePageDataDirty");

    await Promise.all([dispatch("refreshUsers"), dispatch("refreshLogs")]);
  },

  async deleteUser(
    { commit, dispatch }: SuperAdminActionContext,
    userId: number
  ) {
    await superAdminApi.deleteUser(userId);
    commit("markUsersDirty");
    commit("markLogsDirty");
    commit("markHomePageDataDirty");

    await Promise.all([
      dispatch("refreshUsers"),
      dispatch("refreshLogs"),
      dispatch("refreshHomePageData"),
    ]);
  },

  async changeDoctorWorkTime(
    { commit, dispatch }: SuperAdminActionContext,
    payload: {
      user_id: number;
      date: string;
      identifier: "check_in_time" | "check_out_time";
    }
  ) {
    await superAdminApi.changeDoctorWorkTime(payload);
    commit("markWorkTimeRecordsDirty");
    commit("markLogsDirty");
    commit("markHomePageDataDirty");

    await Promise.all([
      dispatch("refreshWorkTimeRecords"),
      dispatch("refreshLogs"),
      dispatch("refreshHomePageData"),
    ]);
  },

  async changeDoctorWorkStatus(
    { commit, dispatch }: SuperAdminActionContext,
    payload: { doctorId: number; status: "online" | "offline" }
  ) {
    await superAdminApi.changeDoctorWorkStatus(payload);
    commit("markUsersDirty");
    commit("markWorkTimeRecordsDirty");
    commit("markLogsDirty");
    commit("markHomePageDataDirty");

    await Promise.all([
      dispatch("refreshUsers"),
      dispatch("refreshWorkTimeRecords"),
      dispatch("refreshLogs"),
      dispatch("refreshHomePageData"),
    ]);
  },

  markUsersDirty({ commit }: SuperAdminActionContext) {
    commit("markUsersDirty");
  },

  markWorkTimeRecordsDirty({ commit }: SuperAdminActionContext) {
    commit("markWorkTimeRecordsDirty");
  },

  markLogsDirty({ commit }: SuperAdminActionContext) {
    commit("markLogsDirty");
  },

  markHomePageDataDirty({ commit }: SuperAdminActionContext) {
    commit("markHomePageDataDirty");
  },
};
