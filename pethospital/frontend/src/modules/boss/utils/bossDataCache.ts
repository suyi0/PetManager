import { BossStockDistribution } from "../api/types";
import { createVersionedLocalCacheAccessors } from "@/shared/utils/versionedLocalCache";

const BOSS_CACHE_OPTIONS = {
  version: 1,
  ttlMs: 1000 * 60 * 60 * 24 * 30,
};

const BOSS_CACHE_KEYS = {
  stockDistribution: "boss:stock-distribution:cache",
};

const bossCache = createVersionedLocalCacheAccessors(BOSS_CACHE_OPTIONS);

/**
 * 从本地缓存读取总裁端股权分布数据。
 * 缓存不存在或格式异常时返回 null。
 */
export const readBossStockDistributionCache = () =>
  bossCache.read<BossStockDistribution>(BOSS_CACHE_KEYS.stockDistribution);

/**
 * 写入总裁端股权分布数据本地缓存。
 */
export const saveBossStockDistributionCache = (
  distribution: BossStockDistribution
) => {
  bossCache.save(BOSS_CACHE_KEYS.stockDistribution, distribution);
};

/**
 * 清空总裁端全部本地业务缓存。
 * 用户退出登录时调用，避免不同账号之间复用旧总裁端数据。
 */
export const clearBossDataCache = () => {
  bossCache.clearAll(Object.values(BOSS_CACHE_KEYS));
};
