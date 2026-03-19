import { ActionContext, ActionTree } from "vuex";
import { profileApi } from "@/modules/user/api/profileApi";
import { State } from "@/store/types";
import { CurrentUserState } from "./types";

// ActionContext 是 Vuex 提供的一个类型，用于在定义 action 时提供类型安全。
// 第一个泛型参数 CurrentUserState：当前模块的状态类型
// 第二个泛型参数 State：根 store 状态类型
type CurrentUserActionContext = ActionContext<CurrentUserState, State>;

type UserEditableField =
  | "userName"
  | "userPhone"
  | "userEmail"
  | "userBirthday"
  | "userAddress"
  | "userHeadImage";

type UserFieldValue = string;

export const currentUserActions: ActionTree<CurrentUserState, State> = {
  // 更新用户数据到后端 API
  updateUserData({ state }: CurrentUserActionContext) {
    if (!state.userName) {
      return Promise.resolve();
    }

    return profileApi.saveUserData({
      name: state.userName,
      phone: state.userPhone,
      email: state.userEmail,
      birthday: state.userBirthday,
      address_id: state.userAddressId,
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
    commit("updateUserField", {
      ...payload,
      userType: rootState.auth.userType,
    });

    dispatch("debouncedUpdateUserData");
  },
};
