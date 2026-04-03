/**
 * 统一解析列表响应，兼容直接返回数组或 { data: [] } 两种格式。
 */
export const unwrapList = <T>(payload: unknown): T[] => {
  if (Array.isArray(payload)) {
    return payload as T[];
  }

  if (payload && typeof payload === "object") {
    const candidate = payload as { data?: unknown };

    if (Array.isArray(candidate.data)) {
      return candidate.data as T[];
    }
  }

  return [];
};

/**
 * 统一解析提示消息，优先读取字符串本身或对象中的 message/data 字段。
 */
export const unwrapMessage = (payload: unknown, fallback: string): string => {
  if (typeof payload === "string") {
    return payload;
  }

  if (payload && typeof payload === "object") {
    const candidate = payload as { message?: unknown; data?: unknown };

    if (typeof candidate.message === "string") {
      return candidate.message;
    }

    if (typeof candidate.data === "string") {
      return candidate.data;
    }
  }

  return fallback;
};
