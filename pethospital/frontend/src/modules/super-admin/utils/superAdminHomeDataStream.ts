import { authStorage } from "@/core/auth/utils/authStorage";
import type { HomePageSummary } from "../api/types";

/**
 * 订阅超级管理员首页实时数据
 */
type HomeDataMessage = {
  event?: string;
  version?: number;
  sentAt?: string;
  data?: HomePageSummary;
};

type HomeDataStreamOptions = {
  onFallbackRefresh?: () => void;
};

/**
 * WebSocket 重试策略，3秒、5秒、10秒、30秒
 */
const RETRY_DELAYS = [3000, 5000, 10000, 30000];
/**
 * 超过 3 次重试后，将触发 fallbackRefresh 回调。
 */
const FALLBACK_RETRY_THRESHOLD = 3;

/**
 * 创建WebSocket连接
 * @param path 后端API路径
 * @returns WebSocket连接
 */
const createWebSocketUrl = (path: string) => {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.host}${path}`;
};

/**
 * 订阅超级管理员首页实时数据。
 * 返回 close 函数，页面卸载时调用以关闭连接。
 */
export const subscribeSuperAdminHomeData = (
  onHomeData: (_summary: HomePageSummary) => void,
  options: HomeDataStreamOptions = {}
) => {
  const token = authStorage.getToken();

  if (!token) {
    return () => undefined;
  }

  let socket: WebSocket | null = null;  // 创建WebSocket连接
  let retryTimer: number | null = null; // 重试定时器
  let retryCount = 0; // 重试次数

  /**
   * 是否由客户端关闭
   */
  let closedByClient = false;

  const connect = () => {
    socket = new WebSocket(
      createWebSocketUrl(
        `/ws/admins/home-data?token=${encodeURIComponent(token)}`
        // encodeURIComponent() 确保 token 中的特殊字符（如 +, /, = 等）被正确编码，避免破坏 URL 结构
      )
    );

    socket.onmessage = (event) => {
      try {
        const message = JSON.parse(event.data) as HomeDataMessage;

        if (message.event === "homeData" && message.data) {
          retryCount = 0;
          onHomeData(message.data);
        }
      } catch {
        // 忽略无法解析的实时消息，等待下一次推送。
      }
    };

    // WebSocket 错误处理
    socket.onerror = () => {
      socket?.close();
    };

    // WebSocket 关闭处理
    socket.onclose = () => {
      socket = null;

      if (!closedByClient) {
        retryCount += 1;

        if (retryCount >= FALLBACK_RETRY_THRESHOLD) {
          options.onFallbackRefresh?.();
        }

        const retryDelay = RETRY_DELAYS[Math.min(retryCount - 1, RETRY_DELAYS.length - 1)];
        retryTimer = window.setTimeout(connect, retryDelay);
      }
    };
  };

  connect();

  return () => {
    closedByClient = true;

    if (retryTimer !== null) {
      window.clearTimeout(retryTimer);
      retryTimer = null;
    }

    socket?.close();
    socket = null;
  };
};
