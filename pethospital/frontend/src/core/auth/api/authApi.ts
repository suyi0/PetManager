import http from "@/api/http";

export const authApi = {
  verify(payload: {
    email?: string;
    phone?: string;
    verificationCode: string;
  }) {
    const requestData: { code: string; email?: string; phone?: string } = {
      code: payload.verificationCode,
    };

    if (payload.email) {
      requestData.email = payload.email;
    } else if (payload.phone) {
      requestData.phone = payload.phone;
    }

    return http.post("/api/auth/verify", { requestData });
  },

  registerSetUser(payload: { email: string; password: string }) {
    return http.post("/api/upload/form", {
      password: payload.password,
      email: payload.email,
    });
  },

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

  checkEmail(payload: { email: string }) {
    return http.post("/api/auth/checkEmail", {
      email: payload.email,
    });
  },

  checkPhone(payload: { phone: string }) {
    return http.post("/api/auth/checkPhone", {
      phone: payload.phone,
    });
  },

  sendVerificationCode(payload: { email?: string; phone?: string }) {
    return http.post("/api/verification/sms/send", {
      email: payload.email,
      phone: payload.phone,
    });
  },

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
};
