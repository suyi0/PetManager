import { UserRow } from "@/modules/super-admin/api/types";
import { createVersionedLocalCacheAccessors } from "@/shared/utils/versionedLocalCache";

const PERSONNEL_CACHE_OPTIONS = {
  version: 1,
  ttlMs: 1000 * 60 * 60 * 24 * 30,
};

const PERSONNEL_CACHE_KEYS = {
  users: "personnel:users:cache",
};

const personnelCache = createVersionedLocalCacheAccessors(
  PERSONNEL_CACHE_OPTIONS
);

/**
 * 从本地缓存读取人事端用户列表。
 * 缓存不存在或格式异常时返回 null；已缓存的空数组会原样返回。
 */
export const readPersonnelUsersCache = () => {
  return personnelCache.readArray<UserRow>(PERSONNEL_CACHE_KEYS.users);
};

/**
 * 写入人事端用户列表本地缓存。
 */
export const savePersonnelUsersCache = (users: UserRow[]) => {
  personnelCache.save(PERSONNEL_CACHE_KEYS.users, users);
};

/**
 * 清空人事端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧人事数据。
 */
export const clearPersonnelDataCache = () => {
  personnelCache.clearAll(Object.values(PERSONNEL_CACHE_KEYS));
};
