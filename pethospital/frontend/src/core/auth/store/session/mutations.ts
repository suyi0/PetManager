import { MutationTree } from "vuex";
import { authStorage } from "@/core/auth/utils/authStorage";
import { isManagementPortalSession } from "@/core/auth/utils/portalAccess";
import { AuthState } from "./types";

const resetAuthState = (state: AuthState) => {
  state.userType = null;
  state.userRole = null;
  state.accountType = null;
  state.positionId = null;
  state.staffKind = null;
  state.permissions = [];
  state.token = null;
  state.isLoggedIn = false;
};

export const authMutations: MutationTree<AuthState> = {
  setSession(
    state,
    payload: {
      token: string;
      userType?: number | null;
      userRole?: string | null;
      accountType?: string | null;
      positionId?: number | null;
      staffKind?: string | null;
      permissions?: string[];
    }
  ) {
    state.userType = payload.userType ?? null;
    state.userRole = payload.userRole ?? null;
    state.accountType = payload.accountType ?? state.accountType ?? null;
    state.positionId = payload.positionId ?? state.positionId ?? null;
    state.staffKind = payload.staffKind ?? state.staffKind ?? null;
    state.permissions = payload.permissions ?? state.permissions ?? [];
    state.token = payload.token;
    state.isLoggedIn = true;
    authStorage.saveSession({
      ...payload,
      // 管理端会话按权限集判定（portal:super-admin 持有者），名字只做展示
      managementSession: isManagementPortalSession({
        permissions: state.permissions,
      }),
    });
  },

  setAccess(
    state,
    payload: {
      accountType?: string | null;
      positionId?: number | null;
      staffKind?: string | null;
      permissions?: string[];
      userType?: number | null;
      userRole?: string | null;
    }
  ) {
    state.accountType = payload.accountType ?? null;
    state.positionId = payload.positionId ?? null;
    state.staffKind = payload.staffKind ?? null;
    state.permissions = payload.permissions ?? [];
    if (payload.userType !== undefined) {
      state.userType = payload.userType;
    }
    if (payload.userRole !== undefined) {
      state.userRole = payload.userRole;
    }
  },

  setLoginStatus(state, status: boolean) {
    state.isLoggedIn = status;
  },

  login(state) {
    state.isLoggedIn = true;
  },

  refreshToken(state, token: string) {
    state.token = token;
    authStorage.updateToken(token);
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
      type_name?: string;
      userRole?: string;
    }
  ) {
    state.userType = data.type_id || data.userType || null;
    state.userRole = data.type_name || data.userRole || null;
    state.isLoggedIn = true;
  },
};
