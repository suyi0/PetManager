<template>
  <section class="page">
    <section class="panel">
      <div class="panel-head">
        <div>
          <h3>库存预警中心</h3>
          <span>按临期和低库存状态处理库存风险</span>
        </div>
        <div class="filter-pills">
          <button
            v-for="filter in filters"
            :key="filter.value"
            type="button"
            :class="{ active: activeFilter === filter.value }"
            @click="activeFilter = filter.value"
          >
            {{ filter.label }}
          </button>
        </div>
      </div>

      <div class="summary-grid">
        <article>
          <small>临期物品</small>
          <strong>{{ expiringCount }}</strong>
          <span>7 天内需要处理</span>
        </article>
        <article>
          <small>低库存</small>
          <strong>{{ lowStockCount }}</strong>
          <span>建议立即补货</span>
        </article>
      </div>

      <div class="warning-list">
        <article
          v-for="item in visibleWarnings"
          :key="item.title + item.description"
          class="warning-row"
        >
          <strong>{{ item.title }}</strong>
          <span>{{ item.description }}</span>
          <i :class="item.level">{{
            item.level === "danger"
              ? "高风险"
              : item.level === "warning"
              ? "优先处理"
              : "复核"
          }}</i>
        </article>
        <div v-if="visibleWarnings.length === 0" class="empty-state">
          当前筛选下没有库存预警
        </div>
      </div>
    </section>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

type WarningFilter = "all" | "danger" | "warning";

export default defineComponent({
  name: "WarehouseAdminWarnings",
  setup() {
    const store = useStore(storeKey);
    const activeFilter = ref<WarningFilter>("all");
    const filters: Array<{ label: string; value: WarningFilter }> = [
      { label: "全部预警", value: "all" },
      { label: "低库存", value: "danger" },
      { label: "临期", value: "warning" },
    ];

    /**
     * 预警页直接复用仓库库存缓存，不再单独重复拉接口。
     */
    const warnings = computed(() =>
      store.state.warehouseAdmin.items
        .flatMap((item) => {
          const rows = [];

          if (item.days_until_expire !== null && item.days_until_expire <= 7) {
            rows.push({
              title: item.item_name,
              description: `临期 ${item.days_until_expire} 天`,
              level: "warning" as const,
            });
          }

          if (item.item_number < 10) {
            rows.push({
              title: item.item_name,
              description: `库存仅 ${item.item_number}`,
              level: "danger" as const,
            });
          }

          return rows;
        })
        .slice(0, 12)
    );

    const visibleWarnings = computed(() => {
      if (activeFilter.value === "all") {
        return warnings.value;
      }

      return warnings.value.filter((item) => item.level === activeFilter.value);
    });

    const expiringCount = computed(
      () =>
        store.state.warehouseAdmin.items.filter(
          (item) =>
            item.days_until_expire !== null && item.days_until_expire <= 7
        ).length
    );

    const lowStockCount = computed(
      () =>
        store.state.warehouseAdmin.items.filter((item) => item.item_number < 10)
          .length
    );

    onMounted(() => {
      void store.dispatch("warehouseAdmin/ensureItems", { force: true });
    });

    return {
      warnings,
      visibleWarnings,
      activeFilter,
      filters,
      expiringCount,
      lowStockCount,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  width: 100%;
  height: 100%;
  box-sizing: border-box;
}

.panel {
  width: 100%;
  box-sizing: border-box;
  border-radius: 12px;
  padding: 18px;
  border: 1px solid #dfe7df;
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(35, 62, 46, 0.06);
}

.panel-head {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 16px;
  margin-bottom: 16px;
}

.panel-head h3 {
  margin: 0 0 4px;
  color: #1d3429;
  font-size: 20px;
}

.panel-head span {
  color: #6d7b72;
  font-size: 12px;
}

.filter-pills {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.filter-pills button {
  padding: 9px 12px;
  border-radius: 8px;
  border: 1px solid #dfe7df;
  background: #ffffff;
  color: #1d3429;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
}

.filter-pills button.active {
  color: #ffffff;
  border-color: #245849;
  background: #245849;
}

.filter-pills button:focus-visible,
.warning-row:focus-within {
  outline: 3px solid rgba(36, 88, 73, 0.24);
  outline-offset: 2px;
}

.summary-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-bottom: 16px;
}

.summary-grid article,
.warning-row {
  padding: 16px;
  border-radius: 10px;
  border: 1px solid #dfe7df;
  background: #ffffff;
}

.summary-grid small,
.summary-grid span,
.warning-row span {
  color: #6d7b72;
}

.summary-grid strong {
  display: block;
  margin: 6px 0;
  font-size: 26px;
  letter-spacing: 0;
}

.summary-grid article:nth-child(1) strong {
  color: #9b6817;
}

.summary-grid article:nth-child(2) strong {
  color: #b04455;
}

.warning-list {
  display: grid;
  gap: 10px;
}

.warning-row {
  display: grid;
  grid-template-columns: 1.2fr 1fr auto;
  align-items: center;
  gap: 16px;
  color: #1d3429;
}

.warning-row i {
  font-style: normal;
  min-width: 76px;
  text-align: center;
  padding: 8px 12px;
  border-radius: 999px;
  font-weight: 700;
  letter-spacing: 0.02em;
}

.warning-row i.warning {
  color: #9b6817;
  background: #fff3d6;
}

.warning-row i.danger {
  color: #b04455;
  background: #ffe9ed;
}

.warning-row i.normal {
  color: #245849;
  background: #e7f1ed;
}

.empty-state {
  padding: 28px;
  border-radius: 10px;
  border: 1px dashed #dfe7df;
  background: #f4f7f4;
  color: #6d7b72;
  text-align: center;
  font-size: 13px;
}

@media (max-width: 1100px) {
  .panel-head,
  .warning-row,
  .summary-grid {
    grid-template-columns: 1fr;
  }
}
</style>
