import { ActionContext, ActionTree } from "vuex";
import { State } from "@/store/types";
import { superAdminApi } from "../api/superAdminApi";
import { SuperAdminState } from "./types";
import { shouldFetch } from "@/store/types";

type SuperAdminActionContext = ActionContext<SuperAdminState, State>;

export const superAdminActions: ActionTree<SuperAdminState, State> = {
  /**
   * 确保用户列表可用。
   * 页面首次进入时会请求，之后优先读缓存；只有脏数据、超时或强制刷新时才再次访问接口。
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
      const homePageData = await superAdminApi.getHomePageData();
      commit("setHomePageData", homePageData);
      return homePageData;
    } finally {
      commit("setHomePageDataLoading", false);
    }
  },

  /**
   * 首页摘要数据单独缓存，不再依赖用户列表和考勤列表拼装。
   */
  async ensureOverviewData({ dispatch }: SuperAdminActionContext) {
    await dispatch("ensureHomePageData");
  },

  async ensureSalaryManagement(
    { state, commit }: SuperAdminActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.salaryManagementMeta, options?.force)) {
      return state.salaryManagement;
    }

    commit("setSalaryManagementLoading", true);
    try {
      const payload = await superAdminApi.getSalaryManagementData();
      commit("setSalaryManagement", payload);
      return payload;
    } finally {
      commit("setSalaryManagementLoading", false);
    }
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

  async refreshSalaryManagement({ dispatch }: SuperAdminActionContext) {
    return dispatch("ensureSalaryManagement", { force: true });
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
};
