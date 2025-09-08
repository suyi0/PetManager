import axios from "axios";

// 创建 axios 实例
const apiClient = axios.create({
  baseURL: "/api", // 设置基础URL
  timeout: 10000, // 设置超时时间
});

// 请求拦截器
apiClient.interceptors.request.use(
  (config) => {
    // 从 localStorage 获取 token
    const token = localStorage.getItem("auth_token");

    // 如果有 token，则添加到请求头中
    if (token) {
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
    // 如果是 401 错误，可能是 token 过期或无效
    if (error.response && error.response.status === 401) {
      // 清除本地存储的 token
      localStorage.removeItem("auth_token");
      // 可以在这里触发登出操作或跳转到登录页
      // store.dispatch('auth/logout');
      // window.location.href = '/login';
    }

    return Promise.reject(error);
  }
);

export default apiClient;
