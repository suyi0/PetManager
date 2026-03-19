import { authStorage } from "@/core/auth/utils/authStorage";
import { AuthState } from "./types";

export const createAuthState = (): AuthState => {
  const persistedSession = authStorage.loadSession();

  return {
    userType: persistedSession.userType,
    token: persistedSession.token,
    isLoggedIn: persistedSession.isLoggedIn,
  };
};
