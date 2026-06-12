/**
 * 写入 localStorage 时统一包裹的缓存数据格式。
 * 各角色端的业务数据不会直接裸存，而是带上版本号和保存时间，
 * 方便后续字段结构变化或缓存过期时自动丢弃旧数据。
 */
type VersionedCacheEnvelope<T> = {
  version: number;
  savedAt: number;
  data: T;
};

/**
 * 某一类业务缓存的版本和有效期配置。
 * version 用于控制缓存结构是否兼容，ttlMs 用于控制本地数据最多复用多久。
 */
type CacheOptions = {
  version: number;
  ttlMs: number;
};

/**
 * 绑定固定缓存配置后生成的读写工具。
 * 角色端通过这组方法读写自己的业务缓存，避免每个模块重复处理版本、过期和数组校验。
 */
type VersionedLocalCacheAccessors = {
  read<T>(_key: string): T | null;
  readArray<T>(_key: string): T[] | null;
  save<T>(_key: string, _value: T): void;
  remove(_key: string): void;
  clearAll(_keys: Iterable<string>): void;
};

/**
 * 判断解析结果是否是可继续检查字段的普通对象。
 * @param value 从 localStorage 解析出的未知数据
 * @returns true 表示可以继续读取其中的 version、savedAt 和 data 字段
 */
const isRecord = (value: unknown): value is Record<string, unknown> =>
  Boolean(value) && typeof value === "object" && !Array.isArray(value);

/**
 * 判断本地缓存是否仍是当前系统认可的版本化缓存格式。
 * @param value JSON.parse 后得到的缓存内容
 * @returns true 表示数据具备版本号、保存时间和业务数据三个核心字段
 */
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
 * 读取某个业务模块的版本化本地缓存。
 * @param key 业务缓存键，例如医生端订单摘要、用户端预约记录等
 * @param options 当前业务缓存对应的版本号和有效期
 * @returns 缓存可复用时返回业务数据；缓存不可用时返回 null
 * @remarks
 * 只要发现版本不匹配、超过有效期、JSON 损坏或仍是旧版裸数据，
 * 就会立即删除该条缓存，让调用方回退到后端请求，避免页面渲染旧结构数据。
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
 * 写入某个业务模块的版本化本地缓存。
 * @param key 业务缓存键
 * @param value 需要持久化的业务数据
 * @param options 当前业务缓存对应的版本号和有效期
 * @remarks 写入时统一补充 version 和 savedAt，业务端只关心自己的数据结构。
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

  localStorage.setItem(key, JSON.stringify(nextValue)); // 写入本地持久存储（键值，对应存储数据）
};

/**
 * 为某个角色端或业务端创建一组本地缓存访问方法。
 * @param options 该业务缓存统一使用的版本号和有效期
 * @returns 已绑定 options 的 read、readArray、save、remove、clearAll 方法
 * @remarks
 * 例如医生端、财务端、用户端都可以各自创建一个访问器，
 * 之后只传业务 key 和类型即可，不需要在每个模块重复写版本校验、过期校验和批量清理。
 */
export const createVersionedLocalCacheAccessors = (
  options: CacheOptions
): VersionedLocalCacheAccessors => {
  /**
   * 读取对象、详情、统计摘要等非固定数组结构的缓存。
   */
  const read = <T>(key: string): T | null =>
    readVersionedLocalCache<T>(key, options);

  /**
   * 读取列表类缓存。
   * 当缓存内容不是数组时返回 null，让列表页重新请求后端，避免错误数据参与渲染。
   */
  const readArray = <T>(key: string): T[] | null => {
    const cachedValue = read<unknown>(key);
    return Array.isArray(cachedValue) ? (cachedValue as T[]) : null;
  };

  /**
   * 使用当前业务端的版本和有效期写入缓存。
   */
  const save = <T>(key: string, value: T) => {
    saveVersionedLocalCache(key, value, options);
  };

  /**
   * 删除单条缓存，常用于只保留一条详情缓存的场景。
   */
  const remove = (key: string) => {
    localStorage.removeItem(key);
  };

  /**
   * 批量删除同一业务端的所有缓存，常用于退出登录或切换账号。
   */
  const clearAll = (keys: Iterable<string>) => {
    Array.from(keys).forEach(remove);
  };

  return {
    read,
    readArray,
    save,
    remove,
    clearAll,
  };
};
