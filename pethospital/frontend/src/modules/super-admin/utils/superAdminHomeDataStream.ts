import { authStorage } from "@/core/auth/utils/authStorage";
import type { HomePageSummary } from "../api/types";

type HomeDataMessage = {
  event?: string;
  version?: number;
  sentAt?: string;
  data?: HomePageSummary;
};

type HomeDataStreamOptions = {
  onFallbackRefresh?: () => void;
};

const RETRY_DELAYS = [3000, 5000, 10000, 30000];
const FALLBACK_RETRY_THRESHOLD = 3;

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

  let socket: WebSocket | null = null;
  let retryTimer: number | null = null;
  let retryCount = 0;
  let closedByClient = false;

  const connect = () => {
    socket = new WebSocket(
      createWebSocketUrl(
        `/ws/admins/home-data?token=${encodeURIComponent(token)}`
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

    socket.onerror = () => {
      socket?.close();
    };

    socket.onclose = () => {
      socket = null;

      if (!closedByClient) {
        retryCount += 1;

        if (retryCount >= FALLBACK_RETRY_THRESHOLD) {
          options.onFallbackRefresh?.();
        }

        const retryDelay =
          RETRY_DELAYS[Math.min(retryCount - 1, RETRY_DELAYS.length - 1)];
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
