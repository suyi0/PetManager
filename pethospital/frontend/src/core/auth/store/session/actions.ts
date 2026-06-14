import { ActionContext, ActionTree } from "vuex";
import { authApi } from "@/core/auth/api/authApi";
import { resolveRoleName } from "@/core/auth/utils/roleUtils";
import { authStorage } from "@/core/auth/utils/authStorage";
import { State } from "@/app/store/types";
import { AuthState } from "./types";

// 定义一个异步函数类型
// TArgs 是一个泛型参数，代表参数数组类型 , extends unknown[] 限定它必须是数组类型
// TResult 是泛型参数，代表函数返回值类型
// ..._args: TArgs 展开参数数组作为函数参数
// => TResult | Promise<TResult> 函数可能返回普通值或Promise
type AsyncFn<TArgs extends unknown[], TResult> = (
  ..._args: TArgs
) => TResult | Promise<TResult>;

// 存储防抖函数的计时器 - WeakMap对象
const debounceTimers = new WeakMap<
  AsyncFn<unknown[], unknown>, // 定时器的ID
  ReturnType<typeof setTimeout> // 返回一个定时器的ID,实际为number
>();

/**
 * 防抖函数 - debounce
 */
function debounce<TArgs extends unknown[], TResult>(
  func: AsyncFn<TArgs, TResult>,
  wait: number
) {
  return (...args: TArgs): Promise<TResult> =>
    new Promise((resolve, reject) => {
      const existingTimer = debounceTimers.get(
        func as AsyncFn<unknown[], unknown>
      );
      if (existingTimer) {
        // 如果已经存在一个定时器，则清除它
        clearTimeout(existingTimer);
      }

      const nextTimer = setTimeout(() => {
        try {
          // Promise.resolve() - 将值转换为Promise对象
          Promise.resolve(func(...args)) // 执行函数func并传入参数...args
            .then(resolve)
            .catch(reject);
        } catch (error) {
          reject(error);
        }
      }, wait);

      debounceTimers.set(func as AsyncFn<unknown[], unknown>, nextTimer);
    });
}

type AuthActionContext = ActionContext<AuthState, State>;

type PortalBridgeSessionPayload = {
  token: string;
  userType: number;
  userRole?: string;
  userName: string;
  userLastName?: string;
  userMiddleName?: string;
  userFirstName?: string;
  userBirthday: string;
  userEmail: string;
  userPhone: string;
  userAddress?: string;
  userHeadImage?: string;
};

const clearAllPortalSessionState = (commit: AuthActionContext["commit"]) => {
  commit("currentUser/clearCurrentUser", undefined, { root: true });
  commit("boss/resetState", undefined, { root: true });
  commit("doctor/resetState", undefined, { root: true });
  commit("finance/resetState", undefined, { root: true });
  commit("personnel/resetState", undefined, { root: true });
  commit("superAdmin/resetState", undefined, { root: true });
  commit("userPortal/resetState", undefined, { root: true });
  commit("warehouseAdmin/resetState", undefined, { root: true });
};

export const authActions: ActionTree<AuthState, State> = {
  verify: debounce(async function (
    _context: AuthActionContext,
    payload: {
      email?: string;
      phone?: string;
      verificationCode: string;
      scene?: "register" | "change";
    }
  ) {
    return authApi
      .verify(payload)
      .then((response) => response)
      .catch((error) => {
        throw error;
      });
  }, 300),

  registerSetUser: debounce(async function (
    _context: AuthActionContext,
    payload: { email: string; password: string }
  ) {
    return authApi
      .registerSetUser(payload)
      .then((response) => {
        if (response.status === 200) {
          return response;
        }
        throw new Error("Register set user failed");
      })
      .catch((error) => {
        throw error;
      });
  },
  300),

  login: debounce(async function (
    { commit }: AuthActionContext,
    payload: {
      identifier: string;
      password: string;
    }
  ) {
    return authApi
      .login(payload)
      .then((response) => {
        const loginData = response.data?.data ?? response.data;
        const user = loginData?.user;
        const token = loginData?.token;

        if (response.status === 200) {
          if (!user || !token) {
            throw new Error("Login response payload is missing user or token");
          }

          const userRole = resolveRoleName(user.type_name, user.type_id);

          commit("setSession", {
            userType: user.type_id,
            userRole,
            token,
          });
          commit(
            "currentUser/setCurrentUser",
            {
              userType: user.type_id,
              userRole,
              userName: user.name,
              userLastName: user.lastName,
              userMiddleName: user.middleName,
              userFirstName: user.firstName,
              userEmail: user.email,
              userPhone: user.phone,
              userBirthday: user.birthday,
              userAddress: user.address,
              userHeadImage: user.head_image,
            },
            { root: true }
          );
          commit("login");

          return response;
        }
        return response;
      })
      .catch((error) => {
        throw error;
      });
  }, 300),

  logout({ commit }: AuthActionContext) {
    clearAllPortalSessionState(commit);
    commit("logout");
  },

  /**
   * 从超级管理员桥接信息恢复目标角色会话。
   * Layout 只负责触发返回动作，会话持久化和 Vuex 状态同步统一放在这里。
   */
  restoreAdminPortalBridgeSession(
    { commit }: AuthActionContext,
    payload: PortalBridgeSessionPayload
  ) {
    authStorage.saveUser(payload);
    commit("setSession", {
      token: payload.token,
      userType: payload.userType,
      userRole: payload.userRole,
    });
    commit(
      "currentUser/setCurrentUser",
      {
        userType: payload.userType,
        userRole: payload.userRole,
        userName: payload.userName,
        userLastName: payload.userLastName,
        userMiddleName: payload.userMiddleName,
        userFirstName: payload.userFirstName,
        userPhone: payload.userPhone,
        userEmail: payload.userEmail,
        userBirthday: payload.userBirthday,
        userAddress: payload.userAddress || "",
        userHeadImage: payload.userHeadImage || "",
      },
      { root: true }
    );
    authStorage.clearAdminPortalBridge();
  },

  expireSession({ commit }: AuthActionContext) {
    clearAllPortalSessionState(commit);
    commit("clearSession");
  },

  checkEmail: debounce(async function (
    _: AuthActionContext,
    payload: { email: string }
  ) {
    return authApi.checkEmail(payload).catch((error) => {
      throw error;
    });
  },
  1000),

  checkPhone: debounce(async function (
    _: AuthActionContext,
    payload: { phone: string }
  ) {
    return authApi.checkPhone(payload).catch((error) => {
      throw error;
    });
  },
  1000),

  sendVerificationCode: debounce(async function (
    _: AuthActionContext,
    payload: { email?: string; phone?: string; scene?: "register" | "change" }
  ) {
    return authApi.sendVerificationCodeRegister(payload).catch((error) => {
      throw error;
    });
  },
  60000),

  checkVerificationCode: debounce(async function (
    _: AuthActionContext,
    payload: { email: string; phone: string; code: string }
  ) {
    return authApi.checkVerificationCode(payload).catch((error) => {
      throw error;
    });
  },
  60000),
  checkLoginStatus({ commit, rootState }: AuthActionContext) {
    if (!rootState.currentUser.userName) {
      return Promise.resolve();
    }

    return authApi
      .checkLoginStatus(rootState.currentUser.userName)
      .then((data) => {
        if (!data) {
          return;
        }

        commit("SET_LOGIN", data);
        if (data.username || data.user) {
          commit(
            "currentUser/updateUserField",
            {
              field: "userName",
              value: data.username || data.user || "",
              userType: rootState.auth.userType,
              userRole: rootState.auth.userRole,
            },
            { root: true }
          );
        }
      });
  },
};
