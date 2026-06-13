import { ActionContext, ActionTree } from "vuex";
import { profileApi } from "@/modules/user/api/userApi";
import { State, shouldFetch } from "@/app/store/types";
import { CurrentUserState } from "./types";

// ActionContext 是 Vuex 提供的一个类型，用于在定义 action 时提供类型安全。
// 第一个泛型参数 CurrentUserState：当前模块的状态类型
// 第二个泛型参数 State：根 store 状态类型
type CurrentUserActionContext = ActionContext<CurrentUserState, State>;

type UserEditableField =
  | "userName"
  | "userLastName"
  | "userMiddleName"
  | "userFirstName"
  | "userPhone"
  | "userEmail"
  | "userBirthday"
  | "userAddress"
  | "userHeadImage";

type UserFieldValue = string;

const buildStorageUserName = (state: CurrentUserState) =>
  [state.userLastName, state.userMiddleName, state.userFirstName]
    .map((part) => String(part || "").trim())
    .filter(Boolean)
    .join("·") ||
  state.userName ||
  "";

export const currentUserActions: ActionTree<CurrentUserState, State> = {
  ensureProfile(
    { state, commit }: CurrentUserActionContext,
    options?: { force?: boolean }
  ) {
    if (!shouldFetch(state.profileMeta, options?.force)) {
      return Promise.resolve(state);
    }

    commit("setProfileLoading", true);
    try {
      commit("hydrateCurrentUserFromStorage");
      return Promise.resolve(state);
    } finally {
      commit("setProfileLoading", false);
    }
  },

  // 更新用户数据到后端 API
  updateUserData({ state }: CurrentUserActionContext) {
    const storageUserName = buildStorageUserName(state);
    if (!storageUserName) {
      return Promise.resolve();
    }

    return profileApi.saveUserData({
      name: storageUserName,
      phone: state.userPhone,
      email: state.userEmail,
      birthday: state.userBirthday,
      address: state.userAddress,
      headImage: state.userHeadImage,
    });
  },
  // 防抖动的更新函数
  debouncedUpdateUserData: (() => {
    let timeoutId: number | null = null;
    return function ({ dispatch }: CurrentUserActionContext, delay = 2000) {
      if (timeoutId) {
        clearTimeout(timeoutId);
      }
      timeoutId = setTimeout(() => {
        dispatch("updateUserData");
        timeoutId = null;
      }, delay) as unknown as number;
    };
  })(),

  // 更新特定用户字段并触发防抖动更新
  updateUserField(
    { commit, dispatch, rootState }: CurrentUserActionContext,
    payload: { field: UserEditableField; value: UserFieldValue }
  ) {
    commit("markProfileDirty");
    commit("updateUserField", {
      ...payload,
      userType: rootState.auth.userType,
      userRole: rootState.auth.userRole,
    });

    dispatch("debouncedUpdateUserData");
  },

  updateUserNameParts(
    { commit, dispatch, rootState }: CurrentUserActionContext,
    payload: {
      userLastName: string;
      userMiddleName: string;
      userFirstName: string;
    }
  ) {
    commit("markProfileDirty");
    commit("updateUserNameParts", {
      ...payload,
      userType: rootState.auth.userType,
      userRole: rootState.auth.userRole,
    });

    dispatch("debouncedUpdateUserData");
  },

  /**
   * 使用邮箱验证码凭证更新当前用户邮箱。
   * 后端返回新 token 后同步刷新登录态，并更新 currentUser 中的邮箱字段。
   */
  async updateEmailWithTicket(
    { commit, rootState }: CurrentUserActionContext,
    payload: { email: string; ticket: string }
  ) {
    const response = await profileApi.updateEmail(payload);
    const token = response.data?.data?.token;

    if (token) {
      commit("auth/refreshToken", token, { root: true });
    }

    commit("updateUserField", {
      field: "userEmail",
      value: payload.email,
      userType: rootState.auth.userType,
      userRole: rootState.auth.userRole,
    });

    return response;
  },
};
