/**
 * 每一类缓存数据都带一份独立元信息。
 * loaded 表示是否成功加载过，dirty 表示是否被业务操作标记为过期，
 * loading 表示当前是否正在请求，lastFetchedAt 用于 TTL 判断。
 */
export interface CacheMeta {
  loaded: boolean;
  dirty: boolean;
  loading: boolean;
  lastFetchedAt: number | null;
}

/**
 * 轻量缓存时效。
 * 页面短时间来回切换时直接复用缓存，停留较久后再自动重拉。
 */
const CACHE_TTL_MS = 60 * 1000;

/**
 * 判断缓存是否已经超过可复用时长。
 * @param lastFetchedAt 上一次成功获取数据的时间戳，未获取过时为 null
 * @returns true 表示缓存已经过期，需要重新请求后端
 */
const isCacheExpired = (lastFetchedAt: number | null): boolean =>
  lastFetchedAt === null || Date.now() - lastFetchedAt > CACHE_TTL_MS;

/**
 * 创建一份默认缓存元信息。
 * @returns 初始缓存标志，表示未加载、未过期、未请求中、没有获取时间
 */
export const createCacheMeta = (): CacheMeta => ({
  loaded: false,
  dirty: false,
  loading: false,
  lastFetchedAt: null,
});

/**
 * 统一决定当前缓存是否需要重新请求。
 * @param meta 当前数据块对应的缓存元信息
 * @param force 是否强制刷新，手动刷新或业务要求立即刷新时传 true
 * @returns true 表示应请求后端；false 表示可以继续复用当前 Vuex 会话缓存
 * @remarks 判断优先级为 force > 未加载 > dirty > TTL 过期。
 */
export const shouldFetch = (meta: CacheMeta, force?: boolean): boolean =>
  force === true ||
  !meta.loaded ||
  meta.dirty ||
  isCacheExpired(meta.lastFetchedAt);

/**
 * 标记缓存已经成功加载。
 * @param meta 当前数据块对应的缓存元信息
 * @remarks 在接口请求成功、或当前会话缓存恢复后调用。
 * 会清除 dirty/loading，并刷新 lastFetchedAt。
 */
export const markCacheLoaded = (meta: CacheMeta) => {
  meta.loaded = true;
  meta.dirty = false;
  meta.loading = false;
  meta.lastFetchedAt = Date.now();
};

/**
 * 标记缓存受业务操作影响，需要重新获取。
 * @param meta 当前数据块对应的缓存元信息
 * @remarks 在新增、修改、删除或实时推送提示数据变化后调用。
 * 只标记 dirty，不立刻请求后端，是否重拉交给页面的 ensure/refresh action 判断。
 */
export const markCacheDirty = (meta: CacheMeta) => {
  meta.dirty = true;
};

/**
 * 设置缓存请求中的状态。
 * @param meta 当前数据块对应的缓存元信息
 * @param loading true 表示正在请求，false 表示请求结束
 */
export const setCacheLoading = (meta: CacheMeta, loading: boolean) => {
  meta.loading = loading;
};

/**
 * 重置缓存元信息。
 * @param meta 当前数据块对应的缓存元信息
 * @remarks 登出、切换账号或清空模块状态时使用。
 */
export const resetCacheMeta = (meta: CacheMeta) => {
  Object.assign(meta, createCacheMeta());
};
