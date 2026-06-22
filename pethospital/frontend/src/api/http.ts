import axios from "axios";
import { dispatchAuthExpiredEvent, normalizeHttpError } from "@/api/httpError";
import { authStorage } from "@/core/auth/utils/authStorage";

/**
 * 全局 HTTP 客户端：统一复用超时、鉴权头和登录失效处理逻辑。
 */
const isDesktopClient = process.env.VUE_APP_DESKTOP_CLIENT === "true";
const apiBaseURL =
  process.env.VUE_APP_API_BASE_URL ||
  (isDesktopClient ? "http://localhost:8081" : "");

const http = axios.create({
  baseURL: apiBaseURL,
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

let lastAuthExpiredEventAt = 0;

const notifyAuthExpired = () => {
  const now = Date.now();

  if (now - lastAuthExpiredEventAt < 1500) {
    return;
  }

  lastAuthExpiredEventAt = now;
  dispatchAuthExpiredEvent();
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
    const normalizedError = normalizeHttpError(error);
    const requestNeedsAuth = shouldAttachAuthHeader(error.config?.url);

    // 登录、注册、验证码等公开接口的 401 不应清理已有会话。
    if (normalizedError.status === 401 && requestNeedsAuth) {
      authStorage.clearAuth();
      notifyAuthExpired();
    }

    return Promise.reject(normalizedError);
  }
);

export default http;
