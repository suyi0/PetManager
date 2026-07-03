import { authStorage } from "@/core/auth/utils/authStorage";

type RealtimeStreamOptions = {
  onFallbackRefresh?: () => void;
  fallbackRetryThreshold?: number;
  resolveTokenOnConnect?: boolean;
};

const RETRY_DELAYS = [10000, 30000, 60000, 120000];

/**
 * 统一生成 WebSocket 地址，避免各实时订阅重复处理 http/https 到 ws/wss 的协议映射。
 */
const createWebSocketUrl = (path: string) => {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.host}${path}`;
};

/**
 * 统一计算实时订阅重连延迟，避免各 WebSocket 客户端重复维护相同退避策略。
 */
const getRetryDelay = (retryCount: number) =>
  RETRY_DELAYS[Math.min(retryCount - 1, RETRY_DELAYS.length - 1)];

/**
 * 统一订阅带 token 的实时消息，集中处理 token 拼接、JSON 解析、错误关闭、退避重连和取消清理。
 */
export const subscribeTokenRealtimeStream = <TMessage>(
  path: string,
  handleMessage: (_message: TMessage) => boolean,
  options: RealtimeStreamOptions = {}
) => {
  const capturedToken = options.resolveTokenOnConnect
    ? null
    : authStorage.getToken();

  if (!options.resolveTokenOnConnect && !capturedToken) {
    return () => undefined;
  }

  let socket: WebSocket | null = null;
  let retryTimer: number | null = null;
  let retryCount = 0;
  let closedByClient = false;

  const connect = () => {
    const token = options.resolveTokenOnConnect
      ? authStorage.getToken()
      : capturedToken;

    if (!token) {
      closedByClient = true;
      return;
    }

    socket = new WebSocket(
      createWebSocketUrl(`${path}?token=${encodeURIComponent(token)}`)
    );

    socket.onmessage = (event) => {
      try {
        const message = JSON.parse(event.data) as TMessage;

        if (handleMessage(message)) {
          retryCount = 0;
        }
      } catch {
        // Ignore malformed realtime messages and wait for the next signal.
      }
    };

    socket.onerror = () => {
      socket?.close();
    };

    socket.onclose = () => {
      socket = null;

      if (!closedByClient) {
        retryCount += 1;

        if (
          options.fallbackRetryThreshold !== undefined &&
          retryCount >= options.fallbackRetryThreshold
        ) {
          options.onFallbackRefresh?.();
        }

        retryTimer = window.setTimeout(connect, getRetryDelay(retryCount));
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
