import { BossStockDistribution } from "../api/types";

const BOSS_CACHE_KEYS = {
  stockDistribution: "boss:stock-distribution:cache",
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
 * 从本地缓存读取总裁端股权分布数据。
 * 缓存不存在或格式异常时返回 null。
 */
export const readBossStockDistributionCache = () =>
  readJsonCache<BossStockDistribution>(BOSS_CACHE_KEYS.stockDistribution);

/**
 * 写入总裁端股权分布数据本地缓存。
 */
export const saveBossStockDistributionCache = (
  distribution: BossStockDistribution
) => {
  saveJsonCache(BOSS_CACHE_KEYS.stockDistribution, distribution);
};

/**
 * 清空总裁端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧总裁端数据。
 */
export const clearBossDataCache = () => {
  Object.values(BOSS_CACHE_KEYS).forEach((key) => {
    localStorage.removeItem(key);
  });
};
