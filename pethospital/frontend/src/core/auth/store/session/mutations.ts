import { MutationTree } from "vuex";
import { authStorage } from "@/core/auth/utils/authStorage";
import { AuthState } from "./types";

const resetAuthState = (state: AuthState) => {
  state.userType = null;
  state.token = null;
  state.isLoggedIn = false;
};

export const authMutations: MutationTree<AuthState> = {
  setSession(
    state,
    payload: {
      token: string;
      userType?: number | null;
    }
  ) {
    state.userType = payload.userType ?? null;
    state.token = payload.token;
    state.isLoggedIn = true;
    authStorage.saveSession(payload);
  },

  setLoginStatus(state, status: boolean) {
    state.isLoggedIn = status;
  },

  login(state) {
    state.isLoggedIn = true;
  },

  refreshToken(state, token: string) {
    state.token = token;
    authStorage.updateToken(token, state.userType);
  },

  clearSession(state) {
    resetAuthState(state);
    authStorage.clearUser();
  },

  logout(state) {
    resetAuthState(state);
    authStorage.clearUser();
    window.location.href = "/";
  },

  SET_LOGIN(
    state,
    data: {
      type_id?: number;
      userType?: number;
    }
  ) {
    state.userType = data.type_id || data.userType || null;
    state.isLoggedIn = true;
  },
};
