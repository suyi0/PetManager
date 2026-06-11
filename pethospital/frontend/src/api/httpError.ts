import type { AxiosError } from "axios";

export const HTTP_AUTH_EXPIRED_EVENT = "pet-hospital:auth-expired";

export type HttpErrorKind =
  | "unauthorized"
  | "forbidden"
  | "not-found"
  | "validation"
  | "server"
  | "timeout"
  | "network"
  | "unknown";

export class AppHttpError extends Error {
  status?: number;
  kind: HttpErrorKind;
  details?: unknown;
  originalError: unknown;

  constructor(params: {
    message: string;
    kind: HttpErrorKind;
    status?: number;
    details?: unknown;
    originalError: unknown;
  }) {
    super(params.message);
    this.name = "AppHttpError";
    this.kind = params.kind;
    this.status = params.status;
    this.details = params.details;
    this.originalError = params.originalError;
    Object.setPrototypeOf(this, AppHttpError.prototype);
  }
}

const readStringField = (
  source: Record<string, unknown> | undefined,
  field: string
) => {
  const value = source?.[field];
  return typeof value === "string" && value.trim() ? value.trim() : "";
};

const getResponseMessage = (payload: unknown): string => {
  if (typeof payload === "string") {
    return payload.trim();
  }

  if (!payload || typeof payload !== "object") {
    return "";
  }

  const source = payload as Record<string, unknown>;
  const errorPayload =
    source.error && typeof source.error === "object"
      ? (source.error as Record<string, unknown>)
      : undefined;
  const dataPayload =
    source.data && typeof source.data === "object"
      ? (source.data as Record<string, unknown>)
      : undefined;

  return (
    readStringField(source, "message") ||
    readStringField(source, "details") ||
    readStringField(errorPayload, "message") ||
    readStringField(errorPayload, "details") ||
    readStringField(dataPayload, "message") ||
    readStringField(dataPayload, "details")
  );
};

const getErrorKind = (status?: number, code?: string): HttpErrorKind => {
  if (code === "ECONNABORTED") {
    return "timeout";
  }

  if (!status) {
    return "network";
  }

  if (status === 401) return "unauthorized";
  if (status === 403) return "forbidden";
  if (status === 404) return "not-found";
  if (status === 400 || status === 422) return "validation";
  if (status >= 500) return "server";

  return "unknown";
};

const getDefaultMessage = (kind: HttpErrorKind) => {
  switch (kind) {
    case "unauthorized":
      return "登录状态已失效，请重新登录";
    case "forbidden":
      return "当前账号没有权限执行该操作";
    case "not-found":
      return "请求的数据不存在或接口路径不正确";
    case "validation":
      return "提交的数据不符合要求，请检查后重试";
    case "server":
      return "服务端处理失败，请稍后重试";
    case "timeout":
      return "请求超时，请检查网络后重试";
    case "network":
      return "无法连接服务端，请检查网络或后端服务";
    default:
      return "请求失败，请稍后重试";
  }
};

export const normalizeHttpError = (error: unknown): AppHttpError => {
  if (error instanceof AppHttpError) {
    return error;
  }

  const axiosError = error as AxiosError;
  const status = axiosError.response?.status;
  const kind = getErrorKind(status, axiosError.code);
  const responseMessage = getResponseMessage(axiosError.response?.data);

  return new AppHttpError({
    message: responseMessage || axiosError.message || getDefaultMessage(kind),
    kind,
    status,
    details: axiosError.response?.data,
    originalError: error,
  });
};

export const getHttpErrorMessage = (
  error: unknown,
  fallback = "数据加载失败，请稍后重试"
) => {
  if (error instanceof AppHttpError && error.message) {
    return error.message;
  }

  if (error instanceof Error && error.message) {
    return error.message;
  }

  return fallback;
};

export const dispatchAuthExpiredEvent = () => {
  window.dispatchEvent(new CustomEvent(HTTP_AUTH_EXPIRED_EVENT));
};
