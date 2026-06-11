type VersionedCacheEnvelope<T> = {
  version: number;
  savedAt: number;
  data: T;
};

type CacheOptions = {
  version: number;
  ttlMs: number;
};

const isRecord = (value: unknown): value is Record<string, unknown> =>
  Boolean(value) && typeof value === "object" && !Array.isArray(value);

const isVersionedEnvelope = <T>(
  value: unknown
): value is VersionedCacheEnvelope<T> => {
  if (!isRecord(value)) {
    return false;
  }

  return (
    typeof value.version === "number" &&
    typeof value.savedAt === "number" &&
    Object.prototype.hasOwnProperty.call(value, "data")
  );
};

/**
 * 读取带版本号和过期时间的 localStorage 缓存。
 * 版本不一致、过期、JSON 损坏或旧版裸数据都会被清理并返回 null。
 */
export const readVersionedLocalCache = <T>(
  key: string,
  options: CacheOptions
): T | null => {
  const rawValue = localStorage.getItem(key);

  if (!rawValue) {
    return null;
  }

  try {
    const parsedValue = JSON.parse(rawValue) as unknown;

    if (!isVersionedEnvelope<T>(parsedValue)) {
      localStorage.removeItem(key);
      return null;
    }

    const isVersionMatched = parsedValue.version === options.version;
    const isExpired = Date.now() - parsedValue.savedAt > options.ttlMs;

    if (!isVersionMatched || isExpired) {
      localStorage.removeItem(key);
      return null;
    }

    return parsedValue.data;
  } catch {
    localStorage.removeItem(key);
    return null;
  }
};

/**
 * 写入带版本号和保存时间的 localStorage 缓存。
 */
export const saveVersionedLocalCache = <T>(
  key: string,
  value: T,
  options: CacheOptions
) => {
  const nextValue: VersionedCacheEnvelope<T> = {
    version: options.version,
    savedAt: Date.now(),
    data: value,
  };

  localStorage.setItem(key, JSON.stringify(nextValue));
};
