const ORDER_SEARCH_HISTORY_KEY = "searchHistory";
const ORDER_FAVORITES_KEY = "userOrderFavorites";
const ORDER_DETAIL_PREVIEW_KEY = "userOrderDetailPreview";

const readJsonArray = <T>(key: string): T[] => {
  const raw = localStorage.getItem(key);

  if (!raw) {
    return [];
  }

  try {
    const parsed = JSON.parse(raw);
    return Array.isArray(parsed) ? (parsed as T[]) : [];
  } catch {
    return [];
  }
};

/**
 * 读取用户订单页搜索历史。
 */
export const readOrderSearchHistory = <T>() =>
  readJsonArray<T>(ORDER_SEARCH_HISTORY_KEY);

/**
 * 将搜索关键词写入订单页搜索历史，并限制最大记录数量。
 */
export const saveOrderSearchKeyword = <T extends { id: number }>(
  history: T[],
  keyword: string,
  createHistoryItem: (_keyword: string) => T,
  getDisplayName: (_item: T) => string,
  maxCount: number
) => {
  const nextHistory = [...history];
  const existingIndex = nextHistory.findIndex(
    (item) => getDisplayName(item) === keyword
  );

  if (existingIndex !== -1) {
    const [item] = nextHistory.splice(existingIndex, 1);
    nextHistory.unshift(item);
  } else {
    nextHistory.unshift(createHistoryItem(keyword));

    if (nextHistory.length > maxCount) {
      nextHistory.pop();
    }
  }

  localStorage.setItem(ORDER_SEARCH_HISTORY_KEY, JSON.stringify(nextHistory));
  return nextHistory;
};

/**
 * 清空用户订单页搜索历史。
 */
export const clearOrderSearchHistory = () => {
  localStorage.removeItem(ORDER_SEARCH_HISTORY_KEY);
};

/**
 * 合并写入用户订单页收藏记录。
 */
export const saveOrderFavorites = <T extends { id: number }>(items: T[]) => {
  const favorites = readJsonArray<T>(ORDER_FAVORITES_KEY);
  const merged = [...favorites];

  items.forEach((item) => {
    if (!merged.some((favorite) => favorite.id === item.id)) {
      merged.push(item);
    }
  });

  localStorage.setItem(ORDER_FAVORITES_KEY, JSON.stringify(merged));
  return merged;
};

/**
 * 保存订单或预约详情页跳转前的轻量预览数据。
 */
export const saveOrderDetailPreview = <T extends object>(
  item: T,
  tab: "order" | "reservation"
) => {
  sessionStorage.setItem(
    ORDER_DETAIL_PREVIEW_KEY,
    JSON.stringify({
      ...item,
      tab,
    })
  );
};

/**
 * 读取订单或预约详情页跳转前保存的轻量预览数据。
 */
export const readOrderDetailPreview = <T>() => {
  const raw = sessionStorage.getItem(ORDER_DETAIL_PREVIEW_KEY);

  if (!raw) {
    return null;
  }

  try {
    return JSON.parse(raw) as T;
  } catch {
    return null;
  }
};

/**
 * 清空订单或预约详情页跳转前保存的轻量预览数据。
 */
export const clearOrderDetailPreview = () => {
  sessionStorage.removeItem(ORDER_DETAIL_PREVIEW_KEY);
};
