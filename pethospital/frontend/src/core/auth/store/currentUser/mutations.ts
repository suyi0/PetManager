import { MutationTree } from "vuex";
import { authStorage } from "@/core/auth/utils/authStorage";
import { CurrentUserState, SetCurrentUserPayload } from "./types";
import { createCurrentUserState } from "./state";

const applyLoadedMeta = (meta: CurrentUserState["profileMeta"]) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

const applyDirtyMeta = (meta: CurrentUserState["profileMeta"]) => {
  meta.dirty = true;
};

// 确保地址对象被正确格式化
const normalizeAddress = (payload: SetCurrentUserPayload) => {
  if (typeof payload.userAddress === "object" && payload.userAddress !== null) {
    return (
      [
        payload.userAddress.street,
        payload.userAddress.city,
        payload.userAddress.state,
        payload.userAddress.postalCode,
        payload.userAddress.country,
      ]
        // .filter((任何合法的变量名) => 任何合法的变量名)
        .filter((part) => part)
        .join(", ")
    );
  }

  return payload.userAddress;
};

// 持久化当前用户
const persistCurrentUser = (
  state: CurrentUserState,
  userType?: number | null,
  userRole?: string | null
) => {
  authStorage.saveCurrentUserProfile({
    userType,
    userRole,
    userName: state.userName || "",
    userBirthday: state.userBirthday || "",
    userEmail: state.userEmail || "",
    userPhone: state.userPhone || "",
    userHeadImage: state.userHeadImage,
    userAddressId: state.userAddressId || undefined,
    userAddress: state.userAddress || "",
  });
};

// 重置当前用户状态
const resetCurrentUserState = (state: CurrentUserState) => {
  state.userName = null;
  state.userBirthday = null;
  state.userEmail = null;
  state.userPhone = null;
  state.userAddressId = null;
  state.userAddress = null;
  state.userHeadImage = undefined;
  state.profileMeta.loaded = false;
  state.profileMeta.dirty = false;
  state.profileMeta.loading = false;
  state.profileMeta.lastFetchedAt = null;
};

export const currentUserMutations: MutationTree<CurrentUserState> = {
  setProfileLoading(state, loading: boolean) {
    state.profileMeta.loading = loading;
  },

  // 设置当前用户
  setCurrentUser(state, payload: SetCurrentUserPayload) {
    state.userName = payload.userName;
    state.userPhone = payload.userPhone;
    state.userEmail = payload.userEmail;
    state.userBirthday = payload.userBirthday;
    state.userHeadImage = payload.userHeadImage;
    state.userAddressId = payload.userAddressId ?? null;
    state.userAddress = normalizeAddress(payload);

    persistCurrentUser(state, payload.userType, payload.userRole);
    applyLoadedMeta(state.profileMeta);
  },

  hydrateCurrentUserFromStorage(state) {
    const nextState = createCurrentUserState();
    state.userName = nextState.userName;
    state.userBirthday = nextState.userBirthday;
    state.userEmail = nextState.userEmail;
    state.userPhone = nextState.userPhone;
    state.userAddressId = nextState.userAddressId;
    state.userAddress = nextState.userAddress;
    state.userHeadImage = nextState.userHeadImage;
    state.profileMeta = nextState.profileMeta;
  },

  // 更新当前用户特定字段
  updateUserField(
    state,
    payload: {
      field:
        | "userName"
        | "userPhone"
        | "userEmail"
        | "userBirthday"
        | "userAddress"
        | "userHeadImage";
      value: string;
      userType?: number | null;
      userRole?: string | null;
    }
  ) {
    switch (payload.field) {
      case "userName":
        state.userName = payload.value;
        break;
      case "userPhone":
        state.userPhone = payload.value;
        break;
      case "userEmail":
        state.userEmail = payload.value;
        break;
      case "userBirthday":
        state.userBirthday = payload.value;
        break;
      case "userAddress":
        state.userAddress = payload.value;
        break;
      case "userHeadImage":
        state.userHeadImage = payload.value;
        break;
      default:
        break;
    }

    persistCurrentUser(state, payload.userType, payload.userRole);
    applyLoadedMeta(state.profileMeta);
  },

  markProfileDirty(state) {
    applyDirtyMeta(state.profileMeta);
  },

  // 清空当前用户
  clearCurrentUser(state) {
    resetCurrentUserState(state);
  },
};
