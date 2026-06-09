import axios from "axios";
import { authStorage } from "@/core/auth/utils/authStorage";

/**
 * 全局 HTTP 客户端：统一复用超时、鉴权头和登录失效处理逻辑。
 */
const http = axios.create({
  baseURL: "",
  timeout: 12000,
});

/**
 * 登录、验证码和注册前置校验接口不依赖已有会话，不能自动附带 Bearer token。
 */
const shouldAttachAuthHeader = (url?: string) => {
  if (!url) {
    return true;
  }

  return ![
    "/api/users/sessions",
    "/api/auth/email-availability",
    "/api/email-verification-codes/registrations",
    "/api/email-verification-tickets/registrations",
    "/api/auth/phone-availability",
    "/api/sms-verification-codes",
    "/api/sms-verification-tickets",
  ].some((path) => url.startsWith(path)); // some(...)：数组里只要有一个满足条件，就返回 true
  // startsWith(...)：判断字符串是不是以某段内容开头
};

/**
 * 创建 http 实例，并添加请求和响应拦截器：
 * - 请求拦截器：自动附带 Bearer token（如果存在且接口需要鉴权）。
 */
http.interceptors.request.use((config) => {
  const token = authStorage.getToken();

  // 只有需要鉴权的接口才补 Authorization，避免公开接口被旧 token 污染。
  if (token && shouldAttachAuthHeader(config.url)) {
    config.headers.Authorization = `Bearer ${token}`;
  }

  return config;
});

/**
 * 响应拦截器：统一处理 401 错误，自动清理会话信息。
 * 这样业务模块就不需要重复编写会话失效处理逻辑了。
 */
http.interceptors.response.use(
  (response) => response,
  (error) => {
    const message =
      error.response?.data?.message ||
      error.response?.data?.error?.details ||
      "";
    const shouldClearAuth =
      typeof message === "string" &&
      /token|expired|invalid|signature|Missing or invalid/i.test(message);

    // 只有明确的 token 失效才清理会话；普通权限拒绝不能把总裁跨端会话清掉。
    if (error.response?.status === 401 && shouldClearAuth) {
      authStorage.clearAuth();
    }

    return Promise.reject(error); // 把错误继续抛出去，让调用方自己决定怎么提示用户
  }
);

export default http;
