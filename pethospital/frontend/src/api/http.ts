import axios from "axios";
import { authStorage } from "@/core/auth/utils/authStorage";

const http = axios.create({
  baseURL: "",
  timeout: 12000,
});

const shouldAttachAuthHeader = (url?: string) => {
  if (!url) {
    return true;
  }

  return ![
    "/api/user/login",
    "/api/auth/verify",
    "/api/auth/checkEmail",
    "/api/auth/checkPhone",
    "/api/verification/sms/send",
    "/api/verification/sms/verify",
  ].some((path) => url.startsWith(path));
};

http.interceptors.request.use((config) => {
  const token = authStorage.getToken();

  if (token && shouldAttachAuthHeader(config.url)) {
    config.headers.Authorization = `Bearer ${token}`;
  }

  return config;
});

http.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response?.status === 401) {
      authStorage.clearAuth();
    }

    return Promise.reject(error);
  }
);

export default http;
