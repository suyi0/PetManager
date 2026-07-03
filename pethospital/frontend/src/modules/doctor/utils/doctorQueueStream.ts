import { subscribeTokenRealtimeStream } from "@/shared/utils/realtimeStream";
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

const FALLBACK_RETRY_THRESHOLD = 3;

export const subscribeDoctorQueue = (
  onQueueData: (_items: QueueItem[]) => void,
  options: DoctorQueueStreamOptions = {}
) =>
  subscribeTokenRealtimeStream<DoctorQueueMessage>(
    "/realtime/doctors/queues",
    (message) => {
      if (message.event === "doctorQueue" && Array.isArray(message.data)) {
        onQueueData(normalizeQueueItems(message.data));
        return true;
      }

      return false;
    },
    {
      onFallbackRefresh: options.onFallbackRefresh,
      fallbackRetryThreshold: FALLBACK_RETRY_THRESHOLD,
    }
  );
