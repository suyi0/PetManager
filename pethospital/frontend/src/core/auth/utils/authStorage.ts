// 存储全部信息
const STORAGE_KEYS = {
  token: "auth_token",
  userType: "user_type",
  userName: "user_name",
  userBirthday: "user_birthday",
  userEmail: "user_email",
  userPhone: "user_phone",
  userAddressId: "address_id",
  userAddress: "user_address",
  userHeadImage: "user_head_image",
  reservateYear: "reservate_year",
  reservateMonth: "reservate_month",
  reservateDay: "reservate_day",
  reservateWeekday: "reservate_weekday",
  reservateSlots: "reservate_slots",
  reservateDoctorData: "reservate_doctorData",
  adminPortalBridge: "admin_portal_bridge",
} as const;

// 存储认证相关信息数组
const AUTH_STORAGE_KEYS = [
  STORAGE_KEYS.token,
  STORAGE_KEYS.userType,
  STORAGE_KEYS.userName,
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
  userType: number | null;
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
  token: string | null;
  isLoggedIn: boolean;
};

type PersistedCurrentUser = {
  userName: string | null;
  userBirthday: string | null;
  userEmail: string | null;
  userPhone: string | null;
  userAddressId: string | null;
  userAddress: string | null;
  userHeadImage: string | null;
};

type PersistedReservate = {
  year: string[];
  month: string[];
  day: string[];
  weekday: string[];
  slots: string[][];
  doctorData: {
    id: number;
    name: string;
    specialty: string;
  }[];
};

type AdminPortalBridge = {
  returnTo: string;
  token: string;
  userType: number;
  userName: string;
  userBirthday: string;
  userEmail: string;
  userPhone: string;
  userAddressId?: string;
  userAddress?: string;
  userHeadImage?: string;
};

// 读取 JSON
const readJson = <T>(key: string, fallback: T): T => {
  const raw = localStorage.getItem(key);

  if (!raw) {
    return fallback;
  }

  try {
    return JSON.parse(raw) as T;
  } catch {
    return fallback;
  }
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

    return JSON.parse(atob(padded)) as Record<string, unknown>;
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
  },

  // 从持久化存储加载用户信息
  loadUser(): PersistedUser {
    const token = this.getToken();
    const activeStorage = getActiveAuthStorage();

    if (!token || !activeStorage) {
      return {
        userName: null,
        userType: null,
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

    return {
      userName: activeStorage.getItem(STORAGE_KEYS.userName),
      userType: userTypeRaw ? Number(userTypeRaw) : null,
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
      token: persistedUser.token,
      isLoggedIn: persistedUser.isLoggedIn,
    };
  },

  // 获取当前用户信息
  loadCurrentUser(): PersistedCurrentUser {
    const persistedUser = this.loadUser();

    return {
      userName: persistedUser.userName,
      userBirthday: persistedUser.userBirthday,
      userEmail: persistedUser.userEmail,
      userPhone: persistedUser.userPhone,
      userAddressId: persistedUser.userAddressId,
      userAddress: persistedUser.userAddress,
      userHeadImage: persistedUser.userHeadImage,
    };
  },

  // 保存会话信息到持久化存储
  saveSession(payload: { token: string; userType?: number | null }) {
    const authStorageTarget =
      payload.userType === 1 ? sessionStorage : localStorage;
    const otherStorage =
      authStorageTarget === sessionStorage ? localStorage : sessionStorage;

    clearKeysFromStorage(otherStorage, AUTH_STORAGE_KEYS);
    authStorageTarget.setItem(STORAGE_KEYS.token, payload.token);
    authStorageTarget.setItem(
      STORAGE_KEYS.userType,
      payload.userType?.toString() || ""
    );
  },

  // 保存当前用户信息到持久化存储
  saveCurrentUserProfile(payload: {
    userName: string;
    userBirthday: string;
    userEmail: string;
    userPhone: string;
    userHeadImage?: string;
    userAddressId?: string;
    userAddress?: string;
    userType?: number | null;
  }) {
    const authStorageTarget =
      payload.userType === 1
        ? sessionStorage
        : getActiveAuthStorage() ?? localStorage;

    authStorageTarget.setItem(STORAGE_KEYS.userName, payload.userName);
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
  },

  // 存储用户信息
  saveUser(payload: {
    token: string;
    userType?: number | null;
    userName: string;
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
    });
    this.saveCurrentUserProfile({
      userType: payload.userType,
      userName: payload.userName,
      userBirthday: payload.userBirthday,
      userEmail: payload.userEmail,
      userPhone: payload.userPhone,
      userHeadImage: payload.userHeadImage,
      userAddressId: payload.userAddressId,
      userAddress: payload.userAddress,
    });
  },

  // 更新令牌
  updateToken(token: string, userType?: number | null) {
    const activeStorage =
      userType === 1 ? sessionStorage : getActiveAuthStorage() ?? localStorage;

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

  // 读取预约信息
  loadReservate(): PersistedReservate {
    return {
      year: readJson<string[]>(STORAGE_KEYS.reservateYear, []),
      month: readJson<string[]>(STORAGE_KEYS.reservateMonth, []),
      day: readJson<string[]>(STORAGE_KEYS.reservateDay, []),
      weekday: readJson<string[]>(STORAGE_KEYS.reservateWeekday, []),
      slots: readJson<string[][]>(STORAGE_KEYS.reservateSlots, []),
      doctorData: readJson<
        {
          id: number;
          name: string;
          specialty: string;
        }[]
      >(STORAGE_KEYS.reservateDoctorData, []),
    };
  },

  // 持久化预约信息
  saveReservate(payload: {
    year: string[];
    month: string[];
    day: string[];
    weekday: string[];
    slots: string[][];
  }) {
    localStorage.setItem(
      STORAGE_KEYS.reservateYear,
      JSON.stringify(payload.year)
    );
    localStorage.setItem(
      STORAGE_KEYS.reservateMonth,
      JSON.stringify(payload.month)
    );
    localStorage.setItem(
      STORAGE_KEYS.reservateDay,
      JSON.stringify(payload.day)
    );
    localStorage.setItem(
      STORAGE_KEYS.reservateWeekday,
      JSON.stringify(payload.weekday)
    );
    localStorage.setItem(
      STORAGE_KEYS.reservateSlots,
      JSON.stringify(payload.slots)
    );
  },

  // 持久化医生信息
  saveDoctorData(
    payload: {
      id: number;
      name: string;
      specialty: string;
    }[]
  ) {
    localStorage.setItem(
      STORAGE_KEYS.reservateDoctorData,
      JSON.stringify(payload)
    );
  },
};
