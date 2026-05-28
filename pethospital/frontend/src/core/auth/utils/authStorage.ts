import { isSuperAdminPortalRole } from "./roleUtils";

// 存储全部信息
const STORAGE_KEYS = {
  token: "auth_token",
  userType: "user_type",
  userRole: "user_role",
  userName: "user_name",
  userLastName: "user_last_name",
  userMiddleName: "user_middle_name",
  userFirstName: "user_first_name",
  userBirthday: "user_birthday",
  userEmail: "user_email",
  userPhone: "user_phone",
  userAddressId: "address_id",
  userAddress: "user_address",
  userHeadImage: "user_head_image",
  adminPortalBridge: "admin_portal_bridge",
  bossPortalReturn: "boss_portal_return",
} as const;

// 存储认证相关信息数组
const AUTH_STORAGE_KEYS = [
  STORAGE_KEYS.token,
  STORAGE_KEYS.userType,
  STORAGE_KEYS.userRole,
  STORAGE_KEYS.userName,
  STORAGE_KEYS.userLastName,
  STORAGE_KEYS.userMiddleName,
  STORAGE_KEYS.userFirstName,
  STORAGE_KEYS.userBirthday,
  STORAGE_KEYS.userEmail,
  STORAGE_KEYS.userPhone,
  STORAGE_KEYS.userAddressId,
  STORAGE_KEYS.userAddress,
  STORAGE_KEYS.userHeadImage,
] as const;

// 获取当前存储的认证信息
const getActiveAuthStorage = (): Storage | null => {
  if (sessionStorage.getItem(STORAGE_KEYS.token)) {
    return sessionStorage;
  }

  if (localStorage.getItem(STORAGE_KEYS.token)) {
    return localStorage;
  }

  return null;
};

// 批量清除指定 key 的值
const clearKeysFromStorage = (
  storage: Storage,
  keys: readonly string[]
): void => {
  keys.forEach((key) => {
    storage.removeItem(key);
  });
};

type PersistedUser = {
  userName: string | null;
  userLastName: string | null;
  userMiddleName: string | null;
  userFirstName: string | null;
  userType: number | null;
  userRole: string | null;
  userBirthday: string | null;
  userEmail: string | null;
  userPhone: string | null;
  userAddressId: string | null;
  userAddress: string | null;
  userHeadImage: string | null;
  token: string | null;
  isLoggedIn: boolean;
};

type PersistedSession = {
  userType: number | null;
  userRole: string | null;
  token: string | null;
  isLoggedIn: boolean;
};

type PersistedCurrentUser = {
  userName: string | null;
  userLastName: string | null;
  userMiddleName: string | null;
  userFirstName: string | null;
  userBirthday: string | null;
  userEmail: string | null;
  userPhone: string | null;
  userAddressId: string | null;
  userAddress: string | null;
  userHeadImage: string | null;
};

type AdminPortalBridge = {
  returnTo: string;
  token: string;
  userType: number;
  userRole?: string;
  userName: string;
  userLastName?: string;
  userMiddleName?: string;
  userFirstName?: string;
  userBirthday: string;
  userEmail: string;
  userPhone: string;
  userAddressId?: string;
  userAddress?: string;
  userHeadImage?: string;
};

type BossPortalReturn = {
  returnTo: string;
};

// 解析 token 返回对象
const parseTokenPayload = (token: string): Record<string, unknown> | null => {
  try {
    const [, payload] = token.split(".");

    if (!payload) {
      return null;
    }

    const normalized = payload.replace(/-/g, "+").replace(/_/g, "/");
    const padded = normalized.padEnd(Math.ceil(normalized.length / 4) * 4, "=");
    const binary = atob(padded);
    const bytes = Uint8Array.from(binary, (char) => char.charCodeAt(0));

    return JSON.parse(new TextDecoder().decode(bytes)) as Record<
      string,
      unknown
    >;
  } catch {
    return null;
  }
};

// 判断 token 是否过期
const isTokenExpired = (token: string): boolean => {
  const payload = parseTokenPayload(token);

  if (!payload || typeof payload.exp !== "number") {
    return true;
  }

  const now = Math.floor(Date.now() / 1000);
  return payload.exp <= now;
};

export const authStorage = {
  // 获取 token 的 payload对象
  getTokenPayload(token?: string) {
    const targetToken = token ?? this.getToken();
    if (!targetToken) {
      return null;
    }

    return parseTokenPayload(targetToken);
  },

  // 获取 token 剩余时间
  getTokenRemainingMs() {
    const payload = this.getTokenPayload();

    if (!payload || typeof payload.exp !== "number") {
      return null;
    }

    return Math.max(0, payload.exp * 1000 - Date.now());
  },

  // 获取 token
  getToken() {
    const activeStorage = getActiveAuthStorage();
    const token = activeStorage?.getItem(STORAGE_KEYS.token) ?? null;

    if (!token) {
      return null;
    }

    if (isTokenExpired(token)) {
      this.clearAuth();
      return null;
    }

    return token;
  },

  // 清除 token
  clearToken() {
    localStorage.removeItem(STORAGE_KEYS.token);
    sessionStorage.removeItem(STORAGE_KEYS.token);
  },

  // 清除持久化信息
  clearAuth() {
    clearKeysFromStorage(localStorage, AUTH_STORAGE_KEYS);
    clearKeysFromStorage(sessionStorage, AUTH_STORAGE_KEYS);
    localStorage.removeItem(STORAGE_KEYS.adminPortalBridge);
    sessionStorage.removeItem(STORAGE_KEYS.bossPortalReturn);
  },

  // 从持久化存储加载用户信息
  loadUser(): PersistedUser {
    const token = this.getToken();
    const activeStorage = getActiveAuthStorage();

    if (!token || !activeStorage) {
      return {
        userName: null,
        userLastName: null,
        userMiddleName: null,
        userFirstName: null,
        userType: null,
        userRole: null,
        userBirthday: null,
        userEmail: null,
        userPhone: null,
        userAddressId: null,
        userAddress: null,
        userHeadImage: null,
        token: null,
        isLoggedIn: false,
      };
    }

    const userTypeRaw = activeStorage.getItem(STORAGE_KEYS.userType);
    const payload = parseTokenPayload(token);
    const payloadRole =
      payload && typeof payload.type_name === "string"
        ? payload.type_name
        : null;

    return {
      userName: activeStorage.getItem(STORAGE_KEYS.userName),
      userLastName: activeStorage.getItem(STORAGE_KEYS.userLastName),
      userMiddleName: activeStorage.getItem(STORAGE_KEYS.userMiddleName),
      userFirstName: activeStorage.getItem(STORAGE_KEYS.userFirstName),
      userType: userTypeRaw ? Number(userTypeRaw) : null,
      userRole: activeStorage.getItem(STORAGE_KEYS.userRole) || payloadRole,
      userBirthday: activeStorage.getItem(STORAGE_KEYS.userBirthday),
      userEmail: activeStorage.getItem(STORAGE_KEYS.userEmail),
      userPhone: activeStorage.getItem(STORAGE_KEYS.userPhone),
      userAddressId: activeStorage.getItem(STORAGE_KEYS.userAddressId),
      userAddress: activeStorage.getItem(STORAGE_KEYS.userAddress),
      userHeadImage: activeStorage.getItem(STORAGE_KEYS.userHeadImage),
      token,
      isLoggedIn: true,
    };
  },

  // 获取会话信息
  loadSession(): PersistedSession {
    const persistedUser = this.loadUser();

    return {
      userType: persistedUser.userType,
      userRole: persistedUser.userRole,
      token: persistedUser.token,
      isLoggedIn: persistedUser.isLoggedIn,
    };
  },

  // 获取当前用户信息
  loadCurrentUser(): PersistedCurrentUser {
    const persistedUser = this.loadUser();

    return {
      userName: persistedUser.userName,
      userLastName: persistedUser.userLastName,
      userMiddleName: persistedUser.userMiddleName,
      userFirstName: persistedUser.userFirstName,
      userBirthday: persistedUser.userBirthday,
      userEmail: persistedUser.userEmail,
      userPhone: persistedUser.userPhone,
      userAddressId: persistedUser.userAddressId,
      userAddress: persistedUser.userAddress,
      userHeadImage: persistedUser.userHeadImage,
    };
  },

  // 保存会话信息到持久化存储
  saveSession(payload: {
    token: string;
    userType?: number | null;
    userRole?: string | null;
  }) {
    const authStorageTarget = isSuperAdminPortalRole(payload.userRole)
      ? sessionStorage
      : localStorage;
    const otherStorage =
      authStorageTarget === sessionStorage ? localStorage : sessionStorage;

    clearKeysFromStorage(otherStorage, AUTH_STORAGE_KEYS);
    authStorageTarget.setItem(STORAGE_KEYS.token, payload.token);
    authStorageTarget.setItem(
      STORAGE_KEYS.userType,
      payload.userType?.toString() || ""
    );
    authStorageTarget.setItem(STORAGE_KEYS.userRole, payload.userRole || "");
  },

  // 保存当前用户信息到持久化存储
  saveCurrentUserProfile(payload: {
    userName: string;
    userLastName?: string;
    userMiddleName?: string;
    userFirstName?: string;
    userBirthday: string;
    userEmail: string;
    userPhone: string;
    userHeadImage?: string;
    userAddressId?: string;
    userAddress?: string;
    userType?: number | null;
    userRole?: string | null;
  }) {
    const authStorageTarget = isSuperAdminPortalRole(payload.userRole)
      ? sessionStorage
      : getActiveAuthStorage() ?? localStorage;

    authStorageTarget.setItem(STORAGE_KEYS.userName, payload.userName);
    authStorageTarget.setItem(
      STORAGE_KEYS.userLastName,
      payload.userLastName || ""
    );
    authStorageTarget.setItem(
      STORAGE_KEYS.userMiddleName,
      payload.userMiddleName || ""
    );
    authStorageTarget.setItem(
      STORAGE_KEYS.userFirstName,
      payload.userFirstName || ""
    );
    authStorageTarget.setItem(STORAGE_KEYS.userBirthday, payload.userBirthday);
    authStorageTarget.setItem(STORAGE_KEYS.userEmail, payload.userEmail);
    authStorageTarget.setItem(STORAGE_KEYS.userPhone, payload.userPhone);
    authStorageTarget.setItem(
      STORAGE_KEYS.userHeadImage,
      payload.userHeadImage || ""
    );
    authStorageTarget.setItem(
      STORAGE_KEYS.userAddressId,
      payload.userAddressId || ""
    );
    authStorageTarget.setItem(
      STORAGE_KEYS.userAddress,
      payload.userAddress || ""
    );
    authStorageTarget.setItem(STORAGE_KEYS.userRole, payload.userRole || "");
  },

  // 存储用户信息
  saveUser(payload: {
    token: string;
    userType?: number | null;
    userRole?: string | null;
    userName: string;
    userLastName?: string;
    userMiddleName?: string;
    userFirstName?: string;
    userBirthday: string;
    userEmail: string;
    userPhone: string;
    userHeadImage?: string;
    userAddressId?: string;
    userAddress?: string;
  }) {
    this.saveSession({
      token: payload.token,
      userType: payload.userType,
      userRole: payload.userRole,
    });
    this.saveCurrentUserProfile({
      userType: payload.userType,
      userRole: payload.userRole,
      userName: payload.userName,
      userLastName: payload.userLastName,
      userMiddleName: payload.userMiddleName,
      userFirstName: payload.userFirstName,
      userBirthday: payload.userBirthday,
      userEmail: payload.userEmail,
      userPhone: payload.userPhone,
      userHeadImage: payload.userHeadImage,
      userAddressId: payload.userAddressId,
      userAddress: payload.userAddress,
    });
  },

  // 更新令牌
  updateToken(
    token: string,
    userType?: number | null,
    userRole?: string | null
  ) {
    const activeStorage = isSuperAdminPortalRole(userRole)
      ? sessionStorage
      : getActiveAuthStorage() ?? localStorage;

    activeStorage.setItem(STORAGE_KEYS.token, token);
  },

  // 清除用户信息
  clearUser() {
    const allKeys = Object.values(STORAGE_KEYS);
    clearKeysFromStorage(localStorage, allKeys);
    clearKeysFromStorage(sessionStorage, allKeys);
  },

  loadAdminPortalBridge(): AdminPortalBridge | null {
    const raw = localStorage.getItem(STORAGE_KEYS.adminPortalBridge);

    if (!raw) {
      return null;
    }

    try {
      return JSON.parse(raw) as AdminPortalBridge;
    } catch {
      return null;
    }
  },

  saveAdminPortalBridge(payload: AdminPortalBridge) {
    localStorage.setItem(
      STORAGE_KEYS.adminPortalBridge,
      JSON.stringify(payload)
    );
  },

  clearAdminPortalBridge() {
    localStorage.removeItem(STORAGE_KEYS.adminPortalBridge);
  },

  loadBossPortalReturn(): BossPortalReturn | null {
    const raw = sessionStorage.getItem(STORAGE_KEYS.bossPortalReturn);

    if (!raw) {
      return null;
    }

    try {
      return JSON.parse(raw) as BossPortalReturn;
    } catch {
      return null;
    }
  },

  saveBossPortalReturn(payload: BossPortalReturn) {
    sessionStorage.setItem(
      STORAGE_KEYS.bossPortalReturn,
      JSON.stringify(payload)
    );
  },

  clearBossPortalReturn() {
    sessionStorage.removeItem(STORAGE_KEYS.bossPortalReturn);
  },
};
