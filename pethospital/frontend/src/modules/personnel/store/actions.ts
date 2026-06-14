import { ActionContext, ActionTree } from "vuex";
import { shouldFetch, State } from "@/app/store/types";
import { personnelApi } from "../api/personnelApi";
import { PersonnelState } from "./types";

type PersonnelActionContext = ActionContext<PersonnelState, State>;

export const personnelActions: ActionTree<PersonnelState, State> = {
  /**
   * 确保人事端用户列表可用。
   * 进入对应页面时通过 RESTful 获取一次数据，只复用当前 Vuex 内存缓存。
   */
  async ensureUsers(
    { state, commit }: PersonnelActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.usersMeta, options?.force)) {
      return state.users;
    }

    commit("setUsersLoading", true);
    try {
      const users = await personnelApi.getUsers();
      commit("setUsers", users);
      return users;
    } finally {
      commit("setUsersLoading", false);
    }
  },

  async createDoctor(
    { commit, dispatch }: PersonnelActionContext,
    userId: number
  ) {
    await personnelApi.createDoctor(userId);
    commit("markUsersDirty");
    await dispatch("ensureUsers", { force: true });
  },

  async deleteDoctor(
    { commit, dispatch }: PersonnelActionContext,
    userId: number
  ) {
    await personnelApi.deleteDoctor(userId);
    commit("markUsersDirty");
    await dispatch("ensureUsers", { force: true });
  },

  async createWarehouseManager(
    { commit, dispatch }: PersonnelActionContext,
    userId: number
  ) {
    await personnelApi.createWarehouserManager(userId);
    commit("markUsersDirty");
    await dispatch("ensureUsers", { force: true });
  },

  async deleteWarehouseManager(
    { commit, dispatch }: PersonnelActionContext,
    userId: number
  ) {
    await personnelApi.deleteWarehouserManager(userId);
    commit("markUsersDirty");
    await dispatch("ensureUsers", { force: true });
  },

  markUsersDirty({ commit }: PersonnelActionContext) {
    commit("markUsersDirty");
  },
};
