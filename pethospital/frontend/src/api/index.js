import axios from "axios";
import { authStorage } from "@/core/auth/utils/authStorage";

// 创建 axios 实例
const apiClient = axios.create({
  baseURL: "/api", // 设置基础URL
  timeout: 10000, // 设置超时时间
});

const shouldAttachAuthHeader = (url) => {
  if (!url) {
    return true;
  }

  return ![
    "/user/login",
    "/auth/verify",
    "/auth/checkEmail",
    "/auth/checkPhone",
    "/verification/sms/send",
    "/verification/sms/verify",
  ].some((path) => url.startsWith(path));
};

// 请求拦截器
apiClient.interceptors.request.use(
  (config) => {
    const token = authStorage.getToken();

    if (token && shouldAttachAuthHeader(config.url)) {
      config.headers.Authorization = `Bearer ${token}`;
    }

    return config;
  },
  (error) => {
    return Promise.reject(error);
  }
);

// 响应拦截器
apiClient.interceptors.response.use(
  (response) => {
    return response;
  },
  (error) => {
    if (error.response && error.response.status === 401) {
      authStorage.clearAuth();
    }

    return Promise.reject(error);
  }
);

export default apiClient;
