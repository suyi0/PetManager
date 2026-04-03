import { InjectionKey } from "vue";
import { createStore, Store } from "vuex";
import { currentUserActions } from "@/core/auth/store/currentUser/actions";
import { currentUserMutations } from "@/core/auth/store/currentUser/mutations";
import { createCurrentUserState } from "@/core/auth/store/currentUser/state";
import { CurrentUserState } from "@/core/auth/store/currentUser/types";
import { authActions } from "@/core/auth/store/session/actions";
import { authGetters } from "@/core/auth/store/session/getters";
import { authMutations } from "@/core/auth/store/session/mutations";
import { createAuthState } from "@/core/auth/store/session/state";
import { AuthState } from "@/core/auth/store/session/types";
import { State } from "@/store/types";
import { uiMutations } from "@/store/ui/mutations";
import { createUiState } from "@/store/ui/state";
import { UiState } from "@/store/ui/types";

export type { AuthState } from "@/core/auth/store/session/types";
export type { CurrentUserState } from "@/core/auth/store/currentUser/types";
export type { State } from "@/store/types";
export type { LoginGrade, UiState } from "@/store/ui/types";

export const storeKey: InjectionKey<Store<State>> = Symbol();

export const appStore = createStore<State>({
  modules: {
    auth: {
      namespaced: true,
      state: createAuthState() as AuthState,
      mutations: authMutations,
      actions: authActions,
      getters: authGetters,
    },
    currentUser: {
      namespaced: true,
      state: createCurrentUserState() as CurrentUserState,
      mutations: currentUserMutations,
      actions: currentUserActions,
    },
    ui: {
      namespaced: true,
      state: createUiState() as UiState,
      mutations: uiMutations,
    },
  },
});
