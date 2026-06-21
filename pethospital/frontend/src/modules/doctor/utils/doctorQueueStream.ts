import { authStorage } from "@/core/auth/utils/authStorage";
import { normalizeQueueItems } from "../api/doctorApi";
import type { QueueItem } from "../api/types";

type DoctorQueueMessage = {
  event?: string;
  version?: number;
  sentAt?: string;
  data?: QueueItem[];
};

type DoctorQueueStreamOptions = {
  onFallbackRefresh?: () => void;
};

const RETRY_DELAYS = [10000, 30000, 60000, 120000];
const FALLBACK_RETRY_THRESHOLD = 3;

const createWebSocketUrl = (path: string) => {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.host}${path}`;
};

export const subscribeDoctorQueue = (
  onQueueData: (_items: QueueItem[]) => void,
  options: DoctorQueueStreamOptions = {}
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
        `/realtime/doctors/queues?token=${encodeURIComponent(token)}`
      )
    );

    socket.onmessage = (event) => {
      try {
        const message = JSON.parse(event.data) as DoctorQueueMessage;

        if (message.event === "doctorQueue" && Array.isArray(message.data)) {
          retryCount = 0;
          onQueueData(normalizeQueueItems(message.data));
        }
      } catch {
        // 忽略无法解析的实时消息，等待下一次队列推送。
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
