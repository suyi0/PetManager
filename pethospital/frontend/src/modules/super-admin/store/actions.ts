import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { superAdminApi } from "../api/superAdminApi";
import { SuperAdminState } from "./types";
import { CreateUserPayload, HomePageSummary } from "../api/types";
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
   *
   * 当options?.force == false时，优先复用缓存数据。
   *
   * 当options?.force == true时，强制刷新缓存数据（不进入缓存逻辑）。
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
   * 应用首页实时推送数据。
   * 首页摘要立即同步到 Vuex 和 localStorage，相关列表只标记为脏数据，
   * 等进入对应页面或显式刷新时再重新请求完整列表。
   */
  applyRealtimeHomePageData(
    { commit }: SuperAdminActionContext,
    homeData: HomePageSummary
  ) {
    saveSuperAdminHomePageDataCache(homeData);
    commit("setHomePageData", homeData);
    commit("markUsersDirty");
    commit("markWorkTimeRecordsDirty");
    commit("markLogsDirty");
    return homeData;
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
   * 如：用户列表、考勤记录、日志、首页摘要数据有缓存时，优先复用缓存数据。
   */
  async refreshSuperAdminData({ dispatch }: SuperAdminActionContext) {
    await Promise.all([
      dispatch("ensureUsers", { force: true }),
      dispatch("ensureWorkTimeRecords", { force: true }),
      dispatch("ensureLogs", { force: true }),
      dispatch("ensureHomePageData", { force: true }),
    ]);
  },

  /**
   * 在线医生页显式刷新时，同时刷新医生资料与值班记录。
   */
  async refreshOnlineDoctorsData({ dispatch }: SuperAdminActionContext) {
    await Promise.all([
      dispatch("ensureUsers", { force: true }),
      dispatch("ensureWorkTimeRecords", { force: true }),
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

    await Promise.all([
      dispatch("ensureUsers", { force: true }),
      dispatch("ensureLogs", { force: true }),
    ]);
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
      dispatch("ensureUsers", { force: true }),
      dispatch("ensureLogs", { force: true }),
      dispatch("ensureHomePageData", { force: true }),
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
      dispatch("ensureWorkTimeRecords", { force: true }),
      dispatch("ensureLogs", { force: true }),
      dispatch("ensureHomePageData", { force: true }),
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
      dispatch("ensureUsers", { force: true }),
      dispatch("ensureWorkTimeRecords", { force: true }),
      dispatch("ensureLogs", { force: true }),
      dispatch("ensureHomePageData", { force: true }),
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
