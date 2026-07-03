/**
 * 统一计算分页总页数，避免各列表页面重复维护最少 1 页的边界处理。
 */
export const calculateTotalPages = (totalItems: number, pageSize: number) =>
  Math.max(1, Math.ceil(totalItems / pageSize));

/**
 * 统一截取当前页数据，避免各列表页面重复维护相同的 page/pageSize 切片公式。
 */
export const getPagedItems = <T>(
  items: readonly T[],
  page: number,
  pageSize: number
): T[] => {
  const start = (page - 1) * pageSize;
  return items.slice(start, start + pageSize);
};

/**
 * 统一生成补齐页面布局的占位序号，避免表格和卡片列表重复计算空行/空卡数量。
 */
export const createPlaceholderIndexes = (
  pageSize: number,
  visibleItemCount: number
) =>
  Array.from(
    { length: Math.max(0, pageSize - visibleItemCount) },
    (_, index) => index + 1
  );
