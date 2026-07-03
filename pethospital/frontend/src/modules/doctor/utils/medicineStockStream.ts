import { subscribeTokenRealtimeStream } from "@/shared/utils/realtimeStream";

type MedicineStockMessage = {
  event?: string;
  version?: number;
  sentAt?: string;
};

/**
 * 订阅药品库存实时变更。任一处库存变动（医生开单扣减、仓库新增/编辑/软删）后，
 * 服务端会广播一个 medicineStock 信号；收到即回调 onStockChanged（通常用于重拉当前药品列表）。
 * 信号本身不带数据，刷新动作由调用方决定。返回取消订阅函数。
 */
export const subscribeMedicineStock = (onStockChanged: () => void) =>
  subscribeTokenRealtimeStream<MedicineStockMessage>(
    "/realtime/doctors/medicine-stock",
    (message) => {
      if (message.event === "medicineStock") {
        onStockChanged();
        return true;
      }

      return false;
    }
  );
