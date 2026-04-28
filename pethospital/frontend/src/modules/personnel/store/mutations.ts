import { MutationTree } from "vuex";
import { UserRow } from "@/modules/super-admin/api/types";
import { createPersonnelState } from "./state";
import { PersonnelState } from "./types";

const applyLoadedMeta = (meta: PersonnelState["usersMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

const applyDirtyMeta = (meta: PersonnelState["usersMeta"]) => {
  meta.dirty = true;
};

export const personnelMutations: MutationTree<PersonnelState> = {
  setUsersLoading(state, loading: boolean) {
    state.usersMeta.loading = loading;
  },

  setUsers(state, users: UserRow[]) {
    state.users = users;
    applyLoadedMeta(state.usersMeta);
  },

  markUsersDirty(state) {
    applyDirtyMeta(state.usersMeta);
  },

  resetState(state) {
    Object.assign(state, createPersonnelState());
  },
};
