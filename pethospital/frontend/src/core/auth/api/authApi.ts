import http from "@/api/http";
import type { AxiosResponse } from "axios";

type ApiResponse<T> = {
  success: boolean;
  code: number;
  message: string;
  data: T;
  error: null | {
    type?: string;
    details?: string;
  };
};

export type EmailVerificationScene = "register" | "change";

export type SendVerificationCodeResponseData = {
  sent: boolean;
  channel: "email" | "sms";
  scene?: EmailVerificationScene;
  phone?: string;
};

export type VerifyEmailCodeResponseData = {
  success: boolean;
  ticket?: string;
  token?: string;
};

export type VerifySmsCodeResponseData = {
  token: string;
};

export type VerifyCodeResponseData =
  | VerifyEmailCodeResponseData
  | VerifySmsCodeResponseData;

export type CurrentUserAccess = {
  user_id: number;
  account_type: "customer" | "staff" | string;
  position_id: number | null;
  position_name: string;
  staff_kind: string;
  system_key: string;
  permissions: string[];
  type_id?: number | null;
  type_name?: string;
};

const emailVerificationCodePaths: Record<EmailVerificationScene, string> = {
  register: "/api/email-verification-codes/registrations",
  change: "/api/email-verification-codes/email-changes",
};

const emailVerificationTicketPaths: Record<EmailVerificationScene, string> = {
  register: "/api/email-verification-tickets/registrations",
  change: "/api/email-verification-tickets/email-changes",
};

type LoginStatusPayload = {
  username?: string;
  user?: string;
  type_id?: number;
  userType?: number;
  type_name?: string;
  userRole?: string;
};

type LoginStatusResponse = {
  type: "LOGIN_STATUS_RESPONSE";
  success: boolean;
  data?: unknown;
  error?: string;
};

type UnknownSocketMessage =
  | LoginStatusResponse
  | { type?: string; [key: string]: unknown };

const loginStatusWebSocketUrl =
  process.env.VUE_APP_LOGIN_STATUS_WS_URL ||
  "ws://localhost:18080/ws/login-status";

const isLoginStatusPayload = (value: unknown): value is LoginStatusPayload => {
  if (!value || typeof value !== "object") {
    return false;
  }

  const candidate = value as Record<string, unknown>;
  return (
    (candidate.username === undefined ||
      typeof candidate.username === "string") &&
    (candidate.user === undefined || typeof candidate.user === "string") &&
    (candidate.type_id === undefined ||
      typeof candidate.type_id === "number") &&
    (candidate.userType === undefined ||
      typeof candidate.userType === "number") &&
    (candidate.type_name === undefined ||
      typeof candidate.type_name === "string") &&
    (candidate.userRole === undefined || typeof candidate.userRole === "string")
  );
};

export const authApi = {
  /**
   * 校验邮箱或手机号对应的验证码是否有效。
   */
  verify(payload: {
    email?: string;
    phone?: string;
    verificationCode?: string;
    code?: string;
    scene?: EmailVerificationScene;
  }): Promise<AxiosResponse<ApiResponse<VerifyCodeResponseData>>> {
    const code = payload.verificationCode ?? payload.code;

    if (payload.email) {
      const scene = payload.scene ?? "register";
      return http.post(emailVerificationTicketPaths[scene], {
        email: payload.email,
        code,
      });
    }

    return http.post("/api/sms-verification-tickets", {
      phone: payload.phone,
      code,
    });
  },

  /**
   * 注册阶段提交用户账号基础信息。
   */
  registerSetUser(payload: { email: string; password: string }) {
    return http.post("/api/users/registrations", {
      password: payload.password,
      email: payload.email,
    });
  },

  /**
   * 使用邮箱或手机号登录并返回会话信息。
   */
  login(payload: { identifier: string; password: string }) {
    const requestData: {
      password: string;
      identifier: string;
    } = {
      password: payload.password,
      identifier: payload.identifier,
    };

    return http.post("/api/users/sessions", requestData);
  },

  changePassword(payload: { currentPassword: string; newPassword: string }) {
    // 后端改密成功会返回一枚续签 token（当前设备无缝保持登录，其他端旧 token 仍失效）。
    return http.patch<ApiResponse<{ token?: string }>>(
      "/api/users/me/password",
      payload
    );
  },

  /**
   * 登出：通知后端做服务端吊销（管理端会 bump session-version 让旧 token 立即失效）。
   * 携带当前 Bearer token，后端据此识别用户。
   */
  logout() {
    return http.post("/api/auth/logout");
  },

  getCurrentUserAccess(): Promise<
    AxiosResponse<ApiResponse<CurrentUserAccess>>
  > {
    return http.get("/api/auth/me");
  },

  /**
   * 检查邮箱是否已被注册。
   */
  checkEmail(payload: { email: string }) {
    return http.post("/api/auth/email-availability", {
      email: payload.email,
    });
  },

  /**
   * 检查手机号是否已被注册。
   */
  checkPhone(payload: { phone: string }) {
    return http.post("/api/auth/phone-availability", {
      phone: payload.phone,
    });
  },

  /**
   * 发送邮箱或手机号验证码。
   */
  sendVerificationCodeRegister(payload: {
    email?: string;
    phone?: string;
    scene?: EmailVerificationScene;
  }): Promise<AxiosResponse<ApiResponse<SendVerificationCodeResponseData>>> {
    if (payload.email) {
      const scene = payload.scene ?? "register";
      return http.post(emailVerificationCodePaths[scene], {
        email: payload.email,
      });
    }

    return http.post("/api/sms-verification-codes", {
      phone: payload.phone,
    });
  },

  /**
   * 校验短信验证码。
   */
  checkVerificationCode(payload: {
    email: string;
    phone: string;
    code: string;
  }): Promise<AxiosResponse<ApiResponse<VerifySmsCodeResponseData>>> {
    return http.post("/api/sms-verification-tickets", {
      phone: payload.phone,
      code: payload.code,
    });
  },

  /**
   * 通过登录状态 WebSocket 通道检查当前用户会话是否仍然有效。
   */
  checkLoginStatus(username: string): Promise<LoginStatusPayload | null> {
    return new Promise((resolve) => {
      const socket = new WebSocket(loginStatusWebSocketUrl);

      socket.onopen = () => {
        socket.send(
          JSON.stringify({
            type: "CHECK_LOGIN_STATUS",
            username,
          })
        );
      };

      socket.onmessage = (event) => {
        try {
          const parsed = JSON.parse(event.data) as UnknownSocketMessage;
          if (
            parsed.type === "LOGIN_STATUS_RESPONSE" &&
            parsed.success &&
            isLoginStatusPayload(parsed.data)
          ) {
            resolve(parsed.data);
            return;
          }
        } catch (error) {
          console.error("解析登录状态响应失败:", error);
        } finally {
          socket.close();
        }

        resolve(null);
      };

      socket.onerror = (error) => {
        console.error("WebSocket connection error:", error);
        resolve(null);
      };

      socket.onclose = () => {
        resolve(null);
      };
    });
  },
};
