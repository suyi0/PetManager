import { ActionContext, ActionTree } from "vuex";
import { authApi } from "@/core/auth/api/authApi";
import { profileApi } from "@/modules/user/api/userApi";
import { resolveRoleName } from "@/core/auth/utils/roleUtils";
import { State } from "@/store/types";
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

export const authActions: ActionTree<AuthState, State> = {
  verify: debounce(async function (
    _context: AuthActionContext,
    payload: {
      email?: string;
      phone?: string;
      verificationCode: string;
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
              userEmail: user.email,
              userPhone: user.phone,
              userBirthday: user.birthday,
              userAddressId: user.address_id,
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

  logout({ commit, state, rootState }: AuthActionContext) {
    if (state.isLoggedIn) {
      profileApi
        .saveUserData({
          name: rootState.currentUser.userName || "",
          phone: rootState.currentUser.userPhone || "",
          email: rootState.currentUser.userEmail || "",
          birthday: rootState.currentUser.userBirthday || "",
          address: rootState.currentUser.userAddress || "",
          headImage: rootState.currentUser.userHeadImage || "",
        })
        .catch((error: unknown) => {
          console.error("Failed to save user data before logout:", error);
        });
    }

    commit("currentUser/clearCurrentUser", undefined, { root: true });
    commit("doctor/resetState", undefined, { root: true });
    commit("superAdmin/resetState", undefined, { root: true });
    commit("userPortal/resetState", undefined, { root: true });
    commit("warehouseAdmin/resetState", undefined, { root: true });
    commit("logout");
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
    payload: { email?: string; phone?: string }
  ) {
    return authApi.sendVerificationCode(payload).catch((error) => {
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
