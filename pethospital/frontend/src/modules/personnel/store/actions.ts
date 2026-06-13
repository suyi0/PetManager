import { ActionContext, ActionTree } from "vuex";
import { shouldFetch, State } from "@/app/store/types";
import { personnelApi } from "../api/personnelApi";
import { PersonnelState } from "./types";
import {
  readPersonnelUsersCache,
  savePersonnelUsersCache,
} from "../utils/personnelDataCache";

type PersonnelActionContext = ActionContext<PersonnelState, State>;

export const personnelActions: ActionTree<PersonnelState, State> = {
  /**
   * 确保人事端用户列表可用。
   * 默认优先复用 Vuex 和 localStorage 缓存，只有缓存为空或强制刷新时才请求后端。
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
      if (!options?.force) {
        const cachedUsers = readPersonnelUsersCache();

        if (cachedUsers) {
          commit("setUsers", cachedUsers);
          return cachedUsers;
        }
      }

      const users = await personnelApi.getUsers();
      savePersonnelUsersCache(users);
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
