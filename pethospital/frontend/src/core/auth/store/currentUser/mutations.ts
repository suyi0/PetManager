import { MutationTree } from "vuex";
import { authStorage } from "@/core/auth/utils/authStorage";
import { CurrentUserState, SetCurrentUserPayload } from "./types";

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
};

export const currentUserMutations: MutationTree<CurrentUserState> = {
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
  },

  // 清空当前用户
  clearCurrentUser(state) {
    resetCurrentUserState(state);
  },
};
