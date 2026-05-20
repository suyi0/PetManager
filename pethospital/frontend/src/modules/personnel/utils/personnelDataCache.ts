import { UserRow } from "@/modules/super-admin/api/types";

const PERSONNEL_CACHE_KEYS = {
  users: "personnel:users:cache",
};

const readJsonCache = <T>(key: string): T | null => {
  const rawValue = localStorage.getItem(key);

  if (!rawValue) {
    return null;
  }

  try {
    return JSON.parse(rawValue) as T;
  } catch {
    localStorage.removeItem(key);
    return null;
  }
};

const saveJsonCache = <T>(key: string, value: T) => {
  localStorage.setItem(key, JSON.stringify(value));
};

/**
 * 从本地缓存读取人事端用户列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readPersonnelUsersCache = () => {
  const cachedValue = readJsonCache<unknown>(PERSONNEL_CACHE_KEYS.users);

  return Array.isArray(cachedValue) ? (cachedValue as UserRow[]) : null;
};

/**
 * 写入人事端用户列表本地缓存。
 */
export const savePersonnelUsersCache = (users: UserRow[]) => {
  saveJsonCache(PERSONNEL_CACHE_KEYS.users, users);
};

/**
 * 清空人事端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧人事数据。
 */
export const clearPersonnelDataCache = () => {
  Object.values(PERSONNEL_CACHE_KEYS).forEach((key) => {
    localStorage.removeItem(key);
  });
};
