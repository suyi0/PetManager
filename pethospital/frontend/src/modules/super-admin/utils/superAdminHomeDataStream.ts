import { subscribeTokenRealtimeStream } from "@/shared/utils/realtimeStream";
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
 * WebSocket 重试策略，避免服务未启动或鉴权失败时高频刷后端日志。
 */
const FALLBACK_RETRY_THRESHOLD = 3;

/**
 * 订阅超级管理员首页实时数据。
 * @param onHomeData 首次数据推送
 * @param options 配置项
 * @returns 返回 close 函数，页面卸载时调用以关闭连接。
 */
export const subscribeSuperAdminHomeData = (
  onHomeData: (_summary: HomePageSummary) => void,
  options: HomeDataStreamOptions = {}
) =>
  subscribeTokenRealtimeStream<HomeDataMessage>(
    "/realtime/admins/home-data",
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
