import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { BossStockDistribution, BossStockItem } from "./types";

const normalizeBossStockItem = (
  item: Partial<BossStockItem>,
  index: number
): BossStockItem => ({
  id: Number(item.id ?? index + 1),
  type:
    item.type === "total" || item.type === "remaining" || item.type === "holder"
      ? item.type
      : "holder",
  holder: String(item.holder ?? ""),
  share: Number(item.share ?? 0),
  percentage: Number(item.percentage ?? 0),
});

export const bossApi = {
  async getStock(): Promise<BossStockDistribution> {
    const { data } = await http.get("/api/bosses/stocks");
    const payload = data?.data ?? data;
    const decisionStocks = unwrapList<BossStockItem>(
      payload?.decisionStocks ?? payload?.decision_stocks
    ).map(normalizeBossStockItem);
    const dividendStocks = unwrapList<BossStockItem>(
      payload?.dividendStocks ?? payload?.dividend_stocks
    ).map(normalizeBossStockItem);

    return {
      decisionStocks,
      dividendStocks,
    };
  },
};
