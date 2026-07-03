import { unwrapList } from "@/api/response";

export interface PagedList<T> {
  items: T[];
  total: number;
  page: number;
  pageSize: number;
}

/**
 * 统一解析后端分页列表响应，避免各角色 API 重复处理 data/items/total/page/pageSize 的兼容逻辑。
 */
export const unwrapPagedList = <T>(
  payload: unknown,
  fallback: { page: number; pageSize: number },
  itemsKey = "items"
): PagedList<T> => {
  const data =
    payload && typeof payload === "object" && "data" in payload
      ? (payload as { data?: unknown }).data
      : payload;
  const source =
    data && typeof data === "object" ? (data as Record<string, unknown>) : {};
  const items = unwrapList<T>(source[itemsKey]);

  return {
    items,
    total: Number(source.total ?? items.length),
    page: Number(source.page ?? fallback.page),
    pageSize: Number(source.pageSize ?? fallback.pageSize),
  };
};
