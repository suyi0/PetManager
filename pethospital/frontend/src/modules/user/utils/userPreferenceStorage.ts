const USER_PREFERENCE_KEYS = {
  userOrderSearchHistory: "user-preference:user-orders-search-history",
  userReservationSearchHistory:
    "user-preference:user-reservations-search-history",
  doctorOrderSearchHistory: "user-preference:doctor-orders-search-history",
  doctorReservationSearchHistory:
    "user-preference:doctor-reservations-search-history",
};

export type SearchHistoryScope = "user" | "doctor";
export type SearchHistoryType = "orders" | "reservations";

const getSearchHistoryKey = (
  scope: SearchHistoryScope,
  type: SearchHistoryType
) => {
  if (scope === "doctor") {
    return type === "orders"
      ? USER_PREFERENCE_KEYS.doctorOrderSearchHistory
      : USER_PREFERENCE_KEYS.doctorReservationSearchHistory;
  }

  return type === "orders"
    ? USER_PREFERENCE_KEYS.userOrderSearchHistory
    : USER_PREFERENCE_KEYS.userReservationSearchHistory;
};

const readJsonArray = <T>(key: string): T[] => {
  const rawValue = localStorage.getItem(key);

  if (!rawValue) {
    return [];
  }

  try {
    const parsedValue = JSON.parse(rawValue);
    return Array.isArray(parsedValue) ? (parsedValue as T[]) : [];
  } catch {
    localStorage.removeItem(key);
    return [];
  }
};

/**
 * 读取用户订单页搜索历史。
 * 这类数据属于用户偏好/使用习惯，不参与业务真实性判断。
 */
export const readOrderSearchHistory = <T>(
  scope: SearchHistoryScope = "user",
  type: SearchHistoryType = "orders"
) => readJsonArray<T>(getSearchHistoryKey(scope, type));

/**
 * 将搜索关键词写入订单页搜索历史，并限制最大记录数量。
 */
export const saveOrderSearchKeyword = <T extends { id: number }>(
  history: T[],
  keyword: string,
  createHistoryItem: (_keyword: string) => T,
  getDisplayName: (_item: T) => string,
  maxCount: number,
  scope: SearchHistoryScope = "user",
  type: SearchHistoryType = "orders"
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

  localStorage.setItem(
    getSearchHistoryKey(scope, type),
    JSON.stringify(nextHistory)
  );
  return nextHistory;
};

/**
 * 清空用户订单页搜索历史。
 */
export const clearOrderSearchHistory = (
  scope: SearchHistoryScope = "user",
  type: SearchHistoryType = "orders"
) => {
  localStorage.removeItem(getSearchHistoryKey(scope, type));
};
