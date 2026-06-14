import { ActionContext, ActionTree } from "vuex";
import { State, shouldFetch } from "@/app/store/types";
import { superAdminApi } from "../api/superAdminApi";
import { SuperAdminState } from "./types";
import { CreateUserPayload, HomePageSummary } from "../api/types";

type SuperAdminActionContext = ActionContext<SuperAdminState, State>;

export const superAdminActions: ActionTree<SuperAdminState, State> = {
  /**
   * 确保用户列表可用。
   * 进入对应页面时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
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
      const users = await superAdminApi.getUsers();
      commit("setUsers", users);
      return users;
    } finally {
      commit("setUsersLoading", false);
    }
  },

  /**
   * 确保考勤列表可用。
   * 进入对应页面时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
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
      const records = await superAdminApi.getWorkTimeRecord();
      commit("setWorkTimeRecords", records);
      return records;
    } finally {
      commit("setWorkTimeRecordsLoading", false);
    }
  },

  /**
   * 确保日志列表可用。
   * 进入对应页面时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
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
      const logs = await superAdminApi.getLogs();
      commit("setLogs", logs);
      return logs;
    } finally {
      commit("setLogsLoading", false);
    }
  },

  /**
   * 确保首页摘要数据可用。
   * 首次进入首页时通过 RESTful 获取，后续 WebSocket 推送只更新 Vuex。
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
      const homePageData = await superAdminApi.homePageGetData();
      commit("setHomePageData", homePageData);
      return homePageData;
    } finally {
      commit("setHomePageDataLoading", false);
    }
  },

  /**
   * 应用首页实时推送数据。
   * 首页摘要只同步到 Vuex，相关列表只标记为脏数据，等进入对应页面或显式刷新时再重新请求完整列表。
   */
  applyRealtimeHomePageData(
    { commit }: SuperAdminActionContext,
    homeData: HomePageSummary
  ) {
    commit("setHomePageData", homeData);
    commit("markUsersDirty");
    commit("markWorkTimeRecordsDirty");
    commit("markLogsDirty");
    return homeData;
  },

  /**
   * 在线医生页进入时刷新医生资料与值班记录两份列表。
   */
  async ensureOnlineDoctorsData({ dispatch }: SuperAdminActionContext) {
    await Promise.all([
      dispatch("ensureUsers", { force: true }),
      dispatch("ensureWorkTimeRecords", { force: true }),
    ]);
  },

  /**
   * 超级管理端入口数据预热。
   * 入口只预热首页摘要，完整列表由对应页面进入时通过 RESTful 获取。
   */
  async refreshSuperAdminData({ dispatch }: SuperAdminActionContext) {
    await dispatch("ensureHomePageData", { force: true });
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
