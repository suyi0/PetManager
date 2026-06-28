import { authStorage } from "@/core/auth/utils/authStorage";

type MedicineStockMessage = {
  event?: string;
  version?: number;
  sentAt?: string;
};

const RETRY_DELAYS = [10000, 30000, 60000, 120000];

const createWebSocketUrl = (path: string) => {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.host}${path}`;
};

/**
 * 订阅药品库存实时变更。任一处库存变动（医生开单扣减、仓库新增/编辑/软删）后，
 * 服务端会广播一个 medicineStock 信号；收到即回调 onStockChanged（通常用于重拉当前药品列表）。
 * 信号本身不带数据，刷新动作由调用方决定。返回取消订阅函数。
 */
export const subscribeMedicineStock = (onStockChanged: () => void) => {
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
        `/realtime/doctors/medicine-stock?token=${encodeURIComponent(token)}`
      )
    );

    socket.onmessage = (event) => {
      try {
        const message = JSON.parse(event.data) as MedicineStockMessage;

        if (message.event === "medicineStock") {
          retryCount = 0;
          onStockChanged();
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
