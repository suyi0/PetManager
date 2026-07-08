import { authStorage } from "@/core/auth/utils/authStorage";
import { AuthState } from "./types";

export const createAuthState = (): AuthState => {
  const persistedSession = authStorage.loadSession();

  return {
    userType: persistedSession.userType,
    userRole: persistedSession.userRole,
    accountType: null,
    positionId: null,
    staffKind: null,
    permissions: [],
    token: persistedSession.token,
    isLoggedIn: persistedSession.isLoggedIn,
  };
};
