import http from "@/api/http";

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
  }) {
    const code = payload.verificationCode ?? payload.code;

    if (payload.email) {
      return http.post("/api/verification/email/verify", {
        email: payload.email,
        code,
      });
    }

    return http.post("/api/verification/sms/verify", {
      phone: payload.phone,
      code,
    });
  },

  /**
   * 注册阶段提交用户账号基础信息。
   */
  registerSetUser(payload: { email: string; password: string }) {
    return http.post("/api/upload/form", {
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

    return http.post("/api/user/login", requestData);
  },

  /**
   * 检查邮箱是否已被注册。
   */
  checkEmail(payload: { email: string }) {
    return http.post("/api/auth/checkEmail", {
      email: payload.email,
    });
  },

  /**
   * 检查手机号是否已被注册。
   */
  checkPhone(payload: { phone: string }) {
    return http.post("/api/auth/checkPhone", {
      phone: payload.phone,
    });
  },

  /**
   * 发送邮箱或手机号验证码。
   */
  sendVerificationCode(payload: { email?: string; phone?: string }) {
    if (payload.email) {
      return http.post("/api/verification/ready", {
        email: payload.email,
      });
    }

    return http.post("/api/verification/sms/send", {
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
  }) {
    return http.post("/api/verification/sms/verify", {
      phone: payload.phone,
      code: payload.code,
    });
  },

  /**
   * 通过登录状态 WebSocket 通道检查当前用户会话是否仍然有效。
   */
  checkLoginStatus(username: string): Promise<LoginStatusPayload | null> {
    return new Promise((resolve) => {
      const socket = new WebSocket("ws://localhost:18080/ws/login-status");

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
