import { ActionContext, ActionTree } from "vuex";
import { shouldFetch, State } from "@/store/types";
import { personnelApi } from "../api/personnelApi";
import { PersonnelState } from "./types";

type PersonnelActionContext = ActionContext<PersonnelState, State>;

export const personnelActions: ActionTree<PersonnelState, State> = {
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

  async refreshUsers({ dispatch }: PersonnelActionContext) {
    return dispatch("ensureUsers", { force: true });
  },
};
