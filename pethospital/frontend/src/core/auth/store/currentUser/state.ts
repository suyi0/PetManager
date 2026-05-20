import { authStorage } from "@/core/auth/utils/authStorage";
import { createCacheMeta } from "@/app/store/state";
import { CurrentUserState } from "./types";

export const createCurrentUserState = (): CurrentUserState => {
  const persistedCurrentUser = authStorage.loadCurrentUser();
  const hasPersistedProfile = Boolean(
    persistedCurrentUser.userName ||
      persistedCurrentUser.userPhone ||
      persistedCurrentUser.userEmail ||
      persistedCurrentUser.userBirthday ||
      persistedCurrentUser.userAddress ||
      persistedCurrentUser.userHeadImage
  );

  return {
    userName: persistedCurrentUser.userName,
    userBirthday: persistedCurrentUser.userBirthday,
    userEmail: persistedCurrentUser.userEmail,
    userPhone: persistedCurrentUser.userPhone,
    userAddressId: persistedCurrentUser.userAddressId,
    userAddress: persistedCurrentUser.userAddress,
    userHeadImage: persistedCurrentUser.userHeadImage || undefined,
    profileMeta: {
      ...createCacheMeta(),
      loaded: hasPersistedProfile,
      lastFetchedAt: hasPersistedProfile ? Date.now() : null,
    },
  };
};
