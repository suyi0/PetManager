import { authStorage } from "@/core/auth/utils/authStorage";
import { CurrentUserState } from "./types";

export const createCurrentUserState = (): CurrentUserState => {
  const persistedCurrentUser = authStorage.loadCurrentUser();

  return {
    userName: persistedCurrentUser.userName,
    userBirthday: persistedCurrentUser.userBirthday,
    userEmail: persistedCurrentUser.userEmail,
    userPhone: persistedCurrentUser.userPhone,
    userAddressId: persistedCurrentUser.userAddressId,
    userAddress: persistedCurrentUser.userAddress,
    userHeadImage: persistedCurrentUser.userHeadImage || undefined,
  };
};
