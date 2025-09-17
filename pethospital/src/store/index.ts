import { createStore, Store, ActionContext } from "vuex";
import { InjectionKey } from "vue";
import axios from "axios";

export type LoginGrade = 1 | 2 | 3 | 4 | 5; // 定义有效的等级值

// 1. 定义 AuthState 类型
export interface AuthState {
  // 添加你的状态属性，例如：
  userName: string | null;
  userBirthday: string | null;
  userEmail: string | null;
  userPhone: string | null;
  userAddressId: string | null;
  userAddress: string | null;
  userHeadImage: string | null;
  token: string | null; // 添加 token 状态
  isLoggedIn: boolean; // 添加登录状态
  personal: boolean; //添加个人信息状态
  showLogin: boolean; // 添加 showLogin 状态
  LoginGrade: LoginGrade | null; // 添加登录页面等级状态
  isLoginButtonActive: boolean; // 添加登录按钮状态
  isLogoutButtonActive: boolean; // 添加登出按钮状态
  isinitLoginActive: boolean; // 添加初始化登录状态
  isAcountLoginButtonActive: boolean; // 添加账号登录按钮状态
  isWeChatLoginButtonActive: boolean; // 添加微信登录按钮状态
  isPhoneLoginButtonActive: boolean; // 添加手机登录按钮状态
  showRegister: boolean; // 添加注册页面状态
  choiceActive: boolean; // 控制同意框状态
}

// 定义 State 类型，包含 AuthState 类型
export interface State {
  auth: AuthState; // 使用 AuthState 类型
}

// 2. 创建 InjectionKey（用于类型安全注入）
export const key: InjectionKey<Store<State>> = Symbol();

// 3. 创建 Vuex Store
export const store = createStore<State>({
  modules: {
    auth: {
      namespaced: true,
      state: {
        userName: localStorage.getItem("user_name") || null, // 从localStorage恢复用户名
        userBirthday: localStorage.getItem("user_birthday") || null, // 从localStorage恢复生日
        userEmail: localStorage.getItem("user_email") || null, // 从localStorage恢复邮箱
        userPhone: localStorage.getItem("user_phone") || null, // 从localStorage恢复手机
        userAddressId: localStorage.getItem("address_id") || null, // 从localStorage恢复地址ID
        userAddress: localStorage.getItem("user_address") || null, // 从localStorage恢复地址
        userHeadImage: localStorage.getItem("user_head_image") || null, // 从localStorage恢复头像
        token: localStorage.getItem("auth_token") || null, // 从localStorage恢复token
        isLoggedIn: !!localStorage.getItem("auth_token"), // 根据是否存在token判断登录状态
        showLogin: false, // 初始登录显示状态
        personal: false, //添加个人信息状态
        isLoginButtonActive: false, // 初始登录按钮状态
        isLogoutButtonActive: false, // 初始登出按钮状态
        isinitLoginActive: false, // 初始登录页面等级状态
        isAcountLoginButtonActive: false, // 初始账号登录按钮状态
        isWeChatLoginButtonActive: false, // 初始微信登录按钮状态
        isPhoneLoginButtonActive: false, // 初始手机登录按钮状态
        LoginGrade: null, // 初始登录页面等级状态
        showRegister: false, // 注册页面状态
        choiceActive: false, // 控制同意框状态
      },
      //mutations 是用来修改 store 状态的方法
      mutations: {
        frontSetUser(
          state: AuthState,
          payload: {
            userName: string;
            userPhone: string;
            userEmail: string;
            userBirthday: string;
            userAddress:
              | {
                  street?: string;
                  city?: string;
                  state?: string;
                  postalCode?: string;
                  country?: string;
                }
              | string;
            userHeadImage: string; // 添加头像参数
            token: string;
            userAddressId?: string;
          }
        ) {
          state.userName = payload.userName;
          state.userPhone = payload.userPhone;
          state.userEmail = payload.userEmail;
          state.userBirthday = payload.userBirthday;
          state.userHeadImage = payload.userHeadImage;

          // 设置 addressId
          if (payload.userAddressId !== undefined) {
            state.userAddressId = payload.userAddressId;
          }

          // 处理地址信息
          if (
            typeof payload.userAddress === "object" &&
            payload.userAddress !== null
          ) {
            // 如果是对象格式，转换为字符串存储
            state.userAddress = [
              payload.userAddress.street,
              payload.userAddress.city,
              payload.userAddress.state,
              payload.userAddress.postalCode,
              payload.userAddress.country,
            ]
              .filter((part) => part)
              .join(", ");
          } else {
            // 如果已经是字符串格式，直接存储
            state.userAddress = payload.userAddress as string;
          }
          state.token = payload.token; // 保存 token
          state.isLoggedIn = true; // 同时更新登录状态
          // 将 token 等信息保存到 localStorage
          localStorage.setItem("auth_token", payload.token);
          localStorage.setItem("user_name", payload.userName);
          localStorage.setItem("user_birthday", payload.userBirthday);
          localStorage.setItem("user_email", payload.userEmail);
          localStorage.setItem("user_phone", payload.userPhone);
          localStorage.setItem("user_head_image", payload.userHeadImage || ""); // 保存头像到localStorage
          localStorage.setItem(
            "address_id",
            payload.userAddressId?.toString() || ""
          );
          localStorage.setItem(
            "user_address",
            payload.userAddress?.toString() || ""
          );
        },
        setLoginStatus(state: AuthState, status: boolean) {
          state.isLoggedIn = status;
        },
        login(state: AuthState) {
          // 添加缺失的login mutation
          state.isLoggedIn = true;
          state.showLogin = false;
        },
        logout(state: AuthState) {
          // 添加缺失的logout mutation
          state.userName = null;
          state.userPhone = null;
          state.userEmail = null;
          state.userBirthday = null;
          state.userAddress = null;
          state.userHeadImage = null; // 清除头像
          state.token = null; // 清除 token
          state.isLoggedIn = false;
          // 从 localStorage 中移除 token
          localStorage.removeItem("auth_token");
          localStorage.removeItem("user_name");
          localStorage.removeItem("user_birthday");
          localStorage.removeItem("user_email");
          localStorage.removeItem("user_phone");
          localStorage.removeItem("user_head_image");
          localStorage.removeItem("user_address");
        },
        SET_LOGIN(state: AuthState, data: any) {
          // 添加缺失的 SET_LOGIN mutation
          state.userName = data.username || data.user;
          state.isLoggedIn = true;
        },
        openLogin(state: AuthState) {
          state.showLogin = true;
        },
        closeLogin(state: AuthState) {
          state.showLogin = false;
        },
        upDataLoginButtonActive(state: AuthState, payload: Partial<AuthState>) {
          Object.assign(state, payload);
        },
        // 添加操作 LoginGardeActive 数组的 mutations
        setLoginGradeActive(state: AuthState, grades: LoginGrade) {
          state.LoginGrade = grades;
          // // 设置整个数组
          // this.$store.commit('auth/setLoginGardeActive', [1, 2, 3, 4, 5]);
        },
        clearLoginGrade(state: AuthState) {
          state.LoginGrade = null;
          // // 清空数组
          // this.$store.commit('auth/clearLoginGarde');
        },
        openPersonal(state: AuthState) {
          state.personal = true;
        },
        closePersonal(state: AuthState) {
          state.personal = false;
        },
      },
      // actions 是用来处理异步操作的方法
      actions: {
        // 模块名需与 dispatch 路径匹配
        // 添加注册 action
        register(
          { commit }: ActionContext<AuthState, State>,
          payload: {
            email: string;
            code: string;
          }
        ) {
          // 在实际应用中，这里应该调用注册 API
          // 用return 返回一个 Promise，这样 action 可以返回一个 Promise，用于处理异步操作.
          // 并且其他组件可以通过返回的 Promise 来处理注册结果
          return (
            axios
              .post("/api/user/verify", {
                email: payload.email,
                code: payload.code,
              })
              //只有2xx状态码才会进入then回调
              .then((response) => {
                if (response.status === 200) {
                  commit("frontSetUser", {
                    username: payload.email,
                    email: payload.email,
                    token: response.data.token, // 从响应中获取 token
                  });
                }
                return response;
              })
              //401这样的错误状态码会直接进入catch回调
              .catch((error) => {
                throw error;
              })
          );
        },
        registerSetUser(
          context: ActionContext<AuthState, State>,
          payload: { email: string; password: string }
        ) {
          axios.post("/api/user/form", {
            password: payload.password,
            email: payload.email,
          });
        },
        login(
          { commit }: ActionContext<AuthState, State>,
          payload: {
            email?: string;
            phone?: string;
            password: string;
          }
        ) {
          // 准备请求数据
          const requestData: any = {
            password: payload.password,
          };

          // 根据传入的参数添加 email 或 phone
          if (payload.email) {
            requestData.email = payload.email;
          } else if (payload.phone) {
            requestData.phone = payload.phone;
          }

          return axios
            .post("/api/user/login", requestData)
            .then((response) => {
              if (response.status === 200 && response.data.success) {
                console.log(response.data);
                commit("frontSetUser", {
                  userName: response.data.user.name, // 从服务器返回的数据中获取用户名
                  userPhone: response.data.user.phone, // 从服务器返回的数据中获取用户电话
                  userEmail: response.data.user.email, // 从服务器返回的数据中获取用户邮箱
                  userBirthday: response.data.user.birthday, // 从服务器返回的数据中获取用户生日
                  userAddressId: response.data.user.address_id, // 从服务器返回的数据中获取用户地址ID
                  userAddress: response.data.user.address, // 从服务器返回的数据中获取用户地址
                  userHeadImage: response.data.user.head_image, // 从服务器返回的数据中获取用户头像
                  token: response.data.token, // 从响应中获取 token
                });
                return response;
              }
            })
            .catch((error) => {
              throw error;
            });
        },
        logout({ commit, state }: ActionContext<AuthState, State>) {
          // 先发送用户数据更新请求
          if (state.userName) {
            axios
              .post("/api/user/form", {
                name: state.userName,
                phone: state.userPhone || "",
                email: state.userEmail || "",
                birthday: state.userBirthday || "",
                address: state.userAddress || "",
                headImage: state.userHeadImage || "",
              })
              .catch((error) => {
                console.error("Failed to save user data before logout:", error);
              });
          }
          // 再执行登出操作
          commit("logout");
        },
        // 更新用户数据到服务器
        updateUserData({ state }: ActionContext<AuthState, State>) {
          // 更新用户数据到服务器
          if (state.userName) {
            axios
              .post("/api/user/form", {
                name: state.userName,
                phone: state.userPhone,
                email: state.userEmail,
                birthday: state.userBirthday,
                address_id: state.userAddressId,
                address: state.userAddress,
                headImage: state.userHeadImage,
              })
              .catch((error) => {
                console.error("Failed to save user data:", error);
              });
          }
        },
        // 添加防抖保存功能
        debouncedUpdateUserData: (() => {
          let timeoutId: number | null = null;
          return function (
            { dispatch }: ActionContext<AuthState, State>,
            delay = 2000
          ) {
            if (timeoutId) {
              clearTimeout(timeoutId);
            }
            timeoutId = setTimeout(() => {
              dispatch("updateUserData");
              timeoutId = null;
            }, delay) as unknown as number;
          };
        })(),
        // 更新前端状态，特定用户字段
        updateUserField(
          { dispatch }: ActionContext<AuthState, State>,
          payload: { field: string; value: any }
        ) {
          // 更新本地状态
          switch (payload.field) {
            case "userName":
              // Deleted:this.state.auth.userName = payload.value;
              break;
            case "userPhone":
              // Deleted:this.state.auth.userPhone = payload.value;
              break;
            case "userEmail":
              // Deleted:this.state.auth.userEmail = payload.value;
              break;
            case "userBirthday":
              // 确保生日字段是有效的日期字符串或空字符串
              // Deleted:this.state.auth.userBirthday =
              // Deleted:payload.value && payload.value.trim() !== "\t"
              // Deleted:? payload.value
              // Deleted:: "";
              break;
            case "userAddress":
              // Deleted:this.state.auth.userAddress = payload.value;
              break;
            case "userHeadImage":
              // Deleted:this.state.auth.userHeadImage = payload.value;
              break;
            default:
          }
          // Deleted:console.log("Updated user field:", payload.field, payload.value);
          // Deleted:console.log(this.state.auth.userPhone);

          // 触发防抖保存
          dispatch("debouncedUpdateUserData");
        },
        // 检查登录状态
        checkLoginStatus({ commit }: ActionContext<State, any>) {
          return new Promise((resolve, reject) => {
            // 首先检查网络连接状态
            if (!navigator.onLine) {
              reject(new Error("网络连接已断开"));
              return;
            }
            // 检查是否在开发环境中且后端服务可能未启动
            const isDevelopment = process.env.NODE_ENV === "development";
            if (
              isDevelopment &&
              !confirm(
                '检测到您正在开发环境中，是否确认后端服务已启动？点击"确定"继续连接，点击"取消"跳过连接。'
              )
            ) {
              reject(new Error("用户选择跳过连接"));
              return;
            }
            let retryCount = 0;
            const maxRetries = 3;
            let ws: WebSocket | null = null;
            let isResolved = false;
            // 从环境变量或配置中获取 WebSocket URL
            const getWebSocketUrl = () => {
              //如果有环境变量则使用环境变量，否则使用默认值
              const baseUrl =
                process.env.VUE_APP_WEBSOCKET_URL ||
                "ws://localhost:8080/websocket";
              return baseUrl;
            };
            // 添加缺失的 handleSuccess 函数
            const handleSuccess = (data: any) => {
              if (!isResolved) {
                isResolved = true;
                resolve(data);
              }
              if (ws) {
                ws.close();
              }
            };
            const handleError = (error: Error) => {
              if (!isResolved) {
                isResolved = true;
                reject(error);
              }
              if (ws) {
                ws.close();
              }
            };
            // 修复：将 handleRetry 函数移到正确位置
            const handleRetry = () => {
              if (ws) {
                ws.close();
                ws = null;
              }
              if (retryCount < maxRetries && !isResolved) {
                retryCount++;
                console.log(
                  `Retrying WebSocket connection
                  (${retryCount}/${maxRetries})`
                );
                // 增加重试间隔，避免过于频繁的连接尝试
                setTimeout(connect, 5000); // 5秒后重试
              } else if (!isResolved) {
                isResolved = true;
                const errorMsg =
                  retryCount >= maxRetries
                    ? `连接服务器失败（已重试${maxRetries}次），请检查网络连接或稍后再试`
                    : "无法连接到服务器，请确保后端服务正在运行";
                reject(new Error(errorMsg));
              }
            };
            const connect = () => {
              try {
                // 检查是否已经解决
                if (isResolved) return;
                // 检查网络连接状态
                if (!navigator.onLine) {
                  handleError(new Error("网络连接已断开"));
                  return;
                }
                // 通过代理连接WebSocket
                const wsUrl = getWebSocketUrl();
                console.log("Attempting to connect to WebSocket at:", wsUrl);
                ws = new WebSocket(wsUrl);
                // 设置连接超时
                const connectTimeout = setTimeout(() => {
                  if (!isResolved) {
                    console.warn("WebSocket connection timeout");
                    if (ws) {
                      ws.close();
                    }
                    handleRetry();
                  }
                }, 10000);
                ws.onopen = () => {
                  clearTimeout(connectTimeout);
                  console.log("WebSocket connected");
                  // 确保连接打开后再发送消息
                  if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({ type: "CHECK_LOGIN_STATUS" }));
                  }
                };
                ws.onmessage = (event) => {
                  let res;
                  try {
                    // 检查 event.data 是否为有效的 JSON 字符串
                    if (
                      typeof event.data === "string" &&
                      event.data.trim() !== ""
                    ) {
                      res = JSON.parse(event.data);
                    } else {
                      console.warn("Received non-JSON data:", event.data);
                      handleError(new Error("Received non-JSON data"));
                      return;
                    }
                    // 处理 JSON 数据
                  } catch (error) {
                    console.error("JSON parse error:", error);
                    // 即使解析失败也认为连接成功，避免频繁重试
                    handleSuccess(null);
                    if (ws) {
                      ws.close();
                    }
                    return;
                  }
                  // 检查响应格式并相应处理
                  if (res.type === "LOGIN_STATUS_RESPONSE") {
                    if (res.success) {
                      commit("SET_LOGIN", res.data);
                      handleSuccess(res.data);
                    } else {
                      handleError(
                        new Error(res.error || "Failed to check login status")
                      );
                    }
                  } else {
                    // 对于其他类型的消息，认为连接成功
                    handleSuccess(null);
                  }
                  if (ws) {
                    ws.close();
                  }
                };
                ws.onerror = (error) => {
                  console.error("WebSocket error:", error);
                  handleError(new Error("WebSocket connection error"));
                };
                ws.onclose = () => {
                  // 如果连接已经解决，则不执行任何操作
                  if (isResolved) return;
                  // 如果没有解决并且没有达到最大重试次数，则重试
                  handleRetry();
                };
              } catch (error) {
                console.error("Connection error:", error);
                handleError(error as Error);
              }
            };
            // 开始连接
            connect();
          });
        },
      },
      getters: {
        formattedUserName(state: AuthState) {
          const fullName = state.userName;
          if (!fullName) return "";

          // 按照·分割获取姓氏和后半部分
          const parts = fullName.split("·");

          // 构建结果字符串
          let result = "";

          // 添加姓氏（第一个部分）
          if (parts[0] && parts[0].trim() !== "") {
            result += parts[0];
          }

          // 添加中间名（第二个部分）
          if (parts[1] && parts[1].trim() !== "") {
            result += parts[1];
          }

          // 添加名字（第三个部分）
          if (parts[2] && parts[2].trim() !== "") {
            result += parts[2];
          }

          return result;
        },
      },
    },
  },
});