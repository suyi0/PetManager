import { subscribeTokenRealtimeStream } from "@/shared/utils/realtimeStream";
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

const FALLBACK_RETRY_THRESHOLD = 3;

/**
 * 订阅财务端首页实时统计数据。
 * 返回 close 函数，页面或布局卸载时调用以关闭连接。
 */
export const subscribeFinanceHomeData = (
  onHomeData: (_summary: FinanceHomeData) => void,
  options: HomeDataStreamOptions = {}
) =>
  subscribeTokenRealtimeStream<HomeDataMessage>(
    "/realtime/finance/home-data",
    (message) => {
      if (message.event === "homeData" && message.data) {
        onHomeData(message.data);
        return true;
      }

      return false;
    },
    {
      onFallbackRefresh: options.onFallbackRefresh,
      fallbackRetryThreshold: FALLBACK_RETRY_THRESHOLD,
    }
  );
