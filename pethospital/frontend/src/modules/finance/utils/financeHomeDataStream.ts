import { authStorage } from "@/core/auth/utils/authStorage";
import type { FinanceHomeData } from "../api/types";

type HomeDataMessage = {
  event?: string;
  version?: number;
  sentAt?: string;
  data?: FinanceHomeData;
};

type HomeDataStreamOptions = {
  onFallbackRefresh?: () => void;
};

const RETRY_DELAYS = [10000, 30000, 60000, 120000];
const FALLBACK_RETRY_THRESHOLD = 3;

/**
 * 根据当前站点协议创建 WebSocket 地址。
 */
const createWebSocketUrl = (path: string) => {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.host}${path}`;
};

/**
 * 订阅财务端首页实时统计数据。
 * 返回 close 函数，页面或布局卸载时调用以关闭连接。
 */
export const subscribeFinanceHomeData = (
  onHomeData: (_summary: FinanceHomeData) => void,
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
        `/realtime/finance/home-data?token=${encodeURIComponent(token)}`
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
