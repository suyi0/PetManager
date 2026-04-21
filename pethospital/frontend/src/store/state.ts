/**
 * 创建一份默认缓存元信息。
 * 页面第一次进入时都是未加载、未脏、没有拉取时间。
 */
export const createCacheMeta = () => ({
  loaded: false,
  dirty: false,
  loading: false,
  lastFetchedAt: null,
});
