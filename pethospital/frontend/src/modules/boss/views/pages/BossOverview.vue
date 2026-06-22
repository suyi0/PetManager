<template>
  <section class="boss-page">
    <div class="boss-grid">
      <article class="hero-card">
        <div class="hero-copy">
          <div class="hero-copy__eyebrow">股份结构总览</div>
        </div>

        <div class="chart-grid">
          <div
            v-for="chart in chartPanels"
            :key="chart.id"
            ref="chartPanelRefs"
            class="chart-panel"
          >
            <div class="chart-panel__head">
              <h4>{{ chart.title }}</h4>
              <span>{{ chart.subtitle }}</span>
            </div>

            <template v-if="chart.hasData">
              <div class="chart-card">
                <svg
                  class="stock-chart"
                  viewBox="0 0 560 560"
                  :aria-label="chart.title"
                >
                  <circle
                    cx="280"
                    cy="280"
                    :r="chartRadius"
                    class="chart-track"
                  />

                  <circle
                    v-for="segment in chart.segments"
                    :key="segment.key"
                    cx="280"
                    cy="280"
                    :r="chartRadius"
                    class="chart-ring"
                    :stroke="segment.color"
                    :stroke-dasharray="`${segment.length} ${
                      circumference - segment.length
                    }`"
                    :stroke-dashoffset="segment.offset"
                    transform="rotate(-90 280 280)"
                    @mouseenter="openHolderCard(chart.id, segment, $event)"
                    @mousemove="moveHolderCard(chart.id, $event)"
                    @mouseleave="closeHolderCard"
                  />

                  <circle cx="280" cy="280" r="132" class="chart-core" />
                  <text
                    x="280"
                    y="255"
                    text-anchor="middle"
                    class="chart-core__label"
                  >
                    已分配股份
                  </text>
                  <text
                    x="280"
                    y="312"
                    text-anchor="middle"
                    class="chart-core__value"
                  >
                    {{ chart.allocatedPercentDisplay }}
                  </text>
                  <text
                    x="280"
                    y="346"
                    text-anchor="middle"
                    class="chart-core__sub"
                  >
                    剩余 {{ chart.remainingPercentDisplay }}
                  </text>
                </svg>

                <transition name="holder-fade">
                  <div
                    v-if="activeHolderCard && activeChartId === chart.id"
                    class="holder-card"
                    :style="holderCardStyle"
                  >
                    <div
                      class="holder-card__swatch"
                      :style="{ background: activeHolderCard.color }"
                    />
                    <div class="holder-card__eyebrow">
                      {{ activeHolderCard.typeLabel }}
                    </div>
                    <h4>{{ activeHolderCard.holder }}</h4>
                    <div class="holder-card__grid">
                      <div>
                        <span>股份编号</span>
                        <strong>#{{ activeHolderCard.id ?? "--" }}</strong>
                      </div>
                      <div>
                        <span>持股份额</span>
                        <strong
                          >{{
                            activeHolderCard.share.toLocaleString("zh-CN")
                          }}
                          股</strong
                        >
                      </div>
                      <div>
                        <span>当前占比</span>
                        <strong
                          >{{ activeHolderCard.percentage.toFixed(2) }}%</strong
                        >
                      </div>
                      <div>
                        <span>基础说明</span>
                        <strong>{{ activeHolderCard.summary }}</strong>
                      </div>
                    </div>
                  </div>
                </transition>
              </div>

              <div class="summary-row">
                <div class="summary-pill">
                  <span>总份额</span>
                  <strong>{{ chart.totalShareDisplay }}</strong>
                </div>
                <div class="summary-pill">
                  <span>剩余份额</span>
                  <strong>{{ chart.remainingShareDisplay }}</strong>
                </div>
              </div>

              <div class="legend-list">
                <div
                  v-for="item in chart.stocks"
                  :key="item.key"
                  class="legend-item"
                >
                  <div class="legend-item__identity">
                    <span
                      class="legend-item__dot"
                      :style="{ background: item.color }"
                    />
                    <div>
                      <strong>{{ item.holder }}</strong>
                      <small>{{ item.typeLabel }}</small>
                    </div>
                  </div>
                  <div class="legend-item__meta">
                    <strong>{{ item.percentage.toFixed(2) }}%</strong>
                    <span>{{ item.share.toLocaleString("zh-CN") }} 股</span>
                  </div>
                </div>
              </div>
            </template>

            <div v-else class="chart-empty">
              <div class="chart-empty__icon">📊</div>
              <strong class="chart-empty__title">暂无股份数据</strong>
              <p class="chart-empty__text">
                尚未登记{{
                  chart.title.replace("分布图", "")
                }}股本，录入后这里将展示持股结构与占比。
              </p>
            </div>
          </div>
        </div>
      </article>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, CSSProperties, defineComponent, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { BossStockItem } from "../../api/types";

type BossOverviewStockItem = BossStockItem & {
  key: string;
  typeLabel: string;
  color: string;
  summary: string;
};

type BossChartPanel = {
  id: "decision" | "dividend";
  title: string;
  subtitle: string;
  stocks: BossOverviewStockItem[];
  segments: Array<
    BossOverviewStockItem & {
      length: number;
      offset: number;
    }
  >;
  totalShareDisplay: string;
  remainingShareDisplay: string;
  remainingPercentDisplay: string;
  allocatedPercentDisplay: string;
  hasData: boolean;
};

const createFallbackStockItem = (): BossOverviewStockItem => ({
  key: "fallback-total",
  id: 0,
  type: "total",
  holder: "总份额",
  share: 0,
  percentage: 100,
  typeLabel: "总股本基准",
  color: "#D87C5A",
  summary: "总股本基准记录",
});

export default defineComponent({
  name: "BossOverview",
  setup() {
    const store = useStore(storeKey);
    const chartPanelRefs = ref<HTMLElement[]>([]);
    const activeHolderCard = ref<BossOverviewStockItem | null>(null);
    const activeChartId = ref<"decision" | "dividend" | null>(null);
    const holderCardStyle = ref<CSSProperties>({
      left: "0px",
      top: "0px",
    });

    const holderPalette = [
      "#D87C5A",
      "#4B7A78",
      "#DAA520",
      "#8F5A9B",
      "#5D88C6",
      "#66A182",
      "#C36F52",
    ];
    const typeLabels: Record<BossStockItem["type"], string> = {
      total: "总股本基准",
      remaining: "待分配",
      holder: "股东份额",
    };

    const buildStocks = (source: BossStockItem[]): BossOverviewStockItem[] => {
      let holderColorIndex = 0;
      return source.map((item, index) => {
        let color = "#D87C5A";
        if (item.type === "remaining") {
          color = "#B9A58E";
        } else if (item.type === "holder") {
          color = holderPalette[holderColorIndex % holderPalette.length];
          holderColorIndex += 1;
        }

        const summary =
          item.type === "total"
            ? "总股本基准记录"
            : item.type === "remaining"
            ? "尚未分配的股份"
            : "当前登记股东份额";

        return {
          ...item,
          key: `${item.type}-${item.id ?? index}-${item.holder}`,
          typeLabel: typeLabels[item.type],
          color,
          summary,
        };
      });
    };

    const decisionStocks = computed(() =>
      buildStocks(store.state.boss.decisionStocks)
    );
    const dividendStocks = computed(() =>
      buildStocks(store.state.boss.dividendStocks)
    );

    const chartRadius = 180;
    const circumference = 2 * Math.PI * chartRadius;

    const createChartPanel = (
      id: "decision" | "dividend",
      title: string,
      subtitle: string,
      stockSource: BossOverviewStockItem[]
    ): BossChartPanel => {
      const totalItem =
        stockSource.find((item) => item.type === "total") ||
        stockSource[0] ||
        createFallbackStockItem();
      const remainingItem =
        stockSource.find((item) => item.type === "remaining") || null;
      const chartItems = stockSource.filter(
        (item) => item.type === "holder" || item.type === "remaining"
      );

      let consumed = 0;
      const segments = chartItems.map((item) => {
        const length = (Math.max(item.percentage, 0) / 100) * circumference;
        const segment = {
          ...item,
          length,
          offset: -consumed,
        };
        consumed += length;
        return segment;
      });

      const remainingPercent = remainingItem?.percentage || 0;
      const allocatedPercent = 100 - remainingPercent;
      // 总股本为 0 视为暂无股份数据：此时百分比是 0/0 退化结果，不应展示「已分配 100%」
      const hasData = Math.round(totalItem.share) > 0;

      return {
        id,
        title,
        subtitle,
        stocks: stockSource,
        segments,
        hasData,
        totalShareDisplay: `${Math.round(totalItem.share).toLocaleString(
          "zh-CN"
        )} 股`,
        remainingShareDisplay: remainingItem
          ? `${Math.round(remainingItem.share).toLocaleString("zh-CN")} 股`
          : "0 股",
        remainingPercentDisplay: `${remainingPercent.toFixed(2)}%`,
        allocatedPercentDisplay: `${allocatedPercent.toFixed(2)}%`,
      };
    };

    const chartPanels = computed(() => [
      createChartPanel(
        "decision",
        "决策股分布图",
        "用于董事会表决、重大事项决策与投票权分布",
        decisionStocks.value
      ),
      createChartPanel(
        "dividend",
        "分红股分布图",
        "用于利润分配、年度分红与收益权比例展示",
        dividendStocks.value
      ),
    ]);

    const updateHolderCardPosition = (
      chartId: "decision" | "dividend",
      event: MouseEvent
    ) => {
      const index = chartId === "decision" ? 0 : 1;
      const container = chartPanelRefs.value[index];
      if (!container) {
        return;
      }

      const rect = container.getBoundingClientRect();
      const cardWidth = 220;
      const cardHeight = 176;
      const offset = 18;
      const preferredLeft = event.clientX - rect.left + offset;
      const preferredTop = event.clientY - rect.top - cardHeight / 2;

      const left = Math.min(
        Math.max(16, preferredLeft),
        rect.width - cardWidth - 16
      );
      const top = Math.min(
        Math.max(16, preferredTop),
        rect.height - cardHeight - 16
      );

      holderCardStyle.value = {
        left: `${left}px`,
        top: `${top}px`,
      };
    };

    const openHolderCard = (
      chartId: "decision" | "dividend",
      segment: BossOverviewStockItem,
      event: MouseEvent
    ) => {
      activeChartId.value = chartId;
      activeHolderCard.value = segment;
      updateHolderCardPosition(chartId, event);
    };

    const moveHolderCard = (
      chartId: "decision" | "dividend",
      event: MouseEvent
    ) => {
      if (!activeHolderCard.value || activeChartId.value !== chartId) {
        return;
      }
      updateHolderCardPosition(chartId, event);
    };

    const closeHolderCard = () => {
      activeHolderCard.value = null;
      activeChartId.value = null;
    };

    onMounted(() => {
      void store.dispatch("boss/ensureStockDistribution", { force: true });
    });

    return {
      chartPanelRefs,
      chartRadius,
      circumference,
      chartPanels,
      activeHolderCard,
      activeChartId,
      holderCardStyle,
      openHolderCard,
      moveHolderCard,
      closeHolderCard,
    };
  },
});
</script>

<style scoped>
.boss-page {
  display: grid;
}

.boss-grid {
  display: grid;
  grid-template-columns: 1fr;
  gap: 22px;
}

.hero-card {
  border-radius: 34px;
  background: rgba(255, 253, 249, 0.9);
  box-shadow: 0 24px 60px rgba(108, 83, 55, 0.14);
  border: 1px solid rgba(111, 82, 51, 0.08);
  padding: 28px;
}

.hero-copy {
  display: flex;
  flex-direction: column;
  gap: 14px;
  margin-bottom: 24px;
}

.hero-copy__eyebrow {
  width: fit-content;
  padding: 8px 12px;
  border-radius: 999px;
  background: #f6ead5;
  color: #906f50;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.hero-copy p {
  margin: 0;
  max-width: 760px;
  color: #7b6651;
  line-height: 1.8;
  font-size: 15px;
}

.chart-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 20px;
}

.chart-panel {
  position: relative;
  padding: 22px;
  border-radius: 28px;
  background: linear-gradient(
    180deg,
    rgba(255, 250, 242, 0.95),
    rgba(249, 240, 224, 0.95)
  );
  border: 1px solid rgba(111, 82, 51, 0.08);
}

.chart-panel__head {
  margin-bottom: 10px;
}

.chart-panel__head h4 {
  margin: 0 0 6px;
  color: #2d2115;
  font-size: 26px;
}

.chart-panel__head span {
  color: #8f7658;
  font-size: 13px;
  line-height: 1.6;
}

.chart-card {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 560px;
}

.chart-empty {
  min-height: 360px;
  display: grid;
  place-content: center;
  justify-items: center;
  gap: 10px;
  padding: 32px 24px;
  text-align: center;
  border: 1px dashed #e3d8c6;
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.5);
}

.chart-empty__icon {
  font-size: 34px;
  opacity: 0.6;
}

.chart-empty__title {
  font-size: 16px;
  font-weight: 700;
  color: #1d3429;
}

.chart-empty__text {
  margin: 0;
  max-width: 320px;
  font-size: 13px;
  line-height: 1.6;
  color: #6d7b72;
}

.stock-chart {
  width: 100%;
  max-width: 520px;
  height: auto;
}

.chart-track {
  fill: none;
  stroke: #ece0cd;
  stroke-width: 60;
}

.chart-ring {
  fill: none;
  stroke-width: 60;
  cursor: pointer;
  transition: stroke-width 0.18s ease, filter 0.18s ease;
}

.chart-ring:hover {
  stroke-width: 66;
  filter: brightness(1.04);
}

.chart-core {
  fill: #fffdf9;
}

.chart-core__label,
.chart-core__sub {
  fill: #8b765c;
  font-size: 18px;
  font-family: Arial, sans-serif;
}

.chart-core__value {
  fill: #241b12;
  font-size: 50px;
  font-family: Georgia, serif;
  font-weight: 700;
}

.summary-row {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-bottom: 16px;
}

.summary-pill {
  padding: 16px 18px;
  border-radius: 20px;
  background: linear-gradient(180deg, #fff8ee, #f8ecdb);
}

.summary-pill span {
  display: block;
  color: #8f7658;
  font-size: 13px;
  margin-bottom: 10px;
}

.summary-pill strong {
  color: #2d2115;
  font-size: 22px;
  font-family: Georgia, serif;
}

.legend-list {
  display: grid;
  gap: 12px;
}

.legend-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 14px;
  border-radius: 20px;
  background: #faf5ec;
}

.legend-item__identity {
  display: flex;
  align-items: center;
  gap: 12px;
}

.legend-item__dot {
  width: 14px;
  height: 14px;
  border-radius: 999px;
  flex: none;
}

.legend-item__identity strong {
  display: block;
  font-size: 17px;
  color: #312419;
}

.legend-item__identity small {
  color: #8b765c;
  font-size: 13px;
}

.legend-item__meta {
  text-align: right;
}

.legend-item__meta strong {
  display: block;
  color: #312419;
  font-size: 22px;
  font-family: Georgia, serif;
}

.legend-item__meta span {
  color: #8b765c;
  font-size: 13px;
}

.holder-card {
  position: absolute;
  z-index: 4;
  width: 220px;
  padding: 18px 18px 16px;
  border-radius: 22px;
  background: rgba(255, 252, 246, 0.96);
  border: 1px solid rgba(120, 91, 60, 0.1);
  box-shadow: 0 22px 48px rgba(93, 71, 48, 0.2);
  backdrop-filter: blur(14px);
}

.holder-card__swatch {
  width: 42px;
  height: 6px;
  border-radius: 999px;
  margin-bottom: 12px;
}

.holder-card__eyebrow {
  color: #8f7658;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.holder-card h4 {
  margin: 10px 0 14px;
  color: #2d2115;
  font-size: 24px;
  font-family: Georgia, serif;
}

.holder-card__grid {
  display: grid;
  grid-template-columns: 1fr;
  gap: 10px;
}

.holder-card__grid span {
  display: block;
  margin-bottom: 4px;
  color: #8f7658;
  font-size: 12px;
}

.holder-card__grid strong {
  color: #2e2217;
  font-size: 14px;
  line-height: 1.45;
}

.holder-fade-enter-active,
.holder-fade-leave-active {
  transition: opacity 0.16s ease, transform 0.16s ease;
}

.holder-fade-enter-from,
.holder-fade-leave-to {
  opacity: 0;
  transform: translateY(6px);
}

@media (max-width: 1280px) {
  .chart-grid {
    grid-template-columns: 1fr;
  }
}
</style>
