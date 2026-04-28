import { BossStockDistribution } from "./types";

export const bossStockDistributionMock: BossStockDistribution = {
  decisionStocks: [
    { id: 1, type: "total", holder: "总份额", share: 1000, percentage: 100 },
    {
      id: 2,
      type: "remaining",
      holder: "剩余份额",
      share: 120,
      percentage: 12,
    },
    { id: 3, type: "holder", holder: "张三", share: 500, percentage: 50 },
    { id: 4, type: "holder", holder: "李四", share: 180, percentage: 18 },
    { id: 5, type: "holder", holder: "王五", share: 120, percentage: 12 },
    { id: 6, type: "holder", holder: "赵六", share: 80, percentage: 8 },
  ],
  dividendStocks: [
    { id: 101, type: "total", holder: "总份额", share: 1000, percentage: 100 },
    {
      id: 102,
      type: "remaining",
      holder: "剩余份额",
      share: 200,
      percentage: 20,
    },
    { id: 103, type: "holder", holder: "张三", share: 420, percentage: 42 },
    { id: 104, type: "holder", holder: "李四", share: 160, percentage: 16 },
    { id: 105, type: "holder", holder: "王五", share: 110, percentage: 11 },
    { id: 106, type: "holder", holder: "赵六", share: 110, percentage: 11 },
  ],
};
