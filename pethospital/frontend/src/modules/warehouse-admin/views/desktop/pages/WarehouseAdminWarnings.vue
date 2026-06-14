<template>
  <section class="page">
    <section class="panel">
      <div class="panel-head">
        <div>
          <h3>库存预警中心</h3>
          <span>Warning Center</span>
        </div>
        <div class="filter-pills">
          <span class="active">全部预警</span>
          <span>低库存</span>
          <span>临期</span>
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
        <article>
          <small>价格异常</small>
          <strong>00</strong>
          <span>单价波动过大</span>
        </article>
      </div>

      <div class="warning-list">
        <article v-for="item in warnings" :key="item.title" class="warning-row">
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
      </div>
    </section>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

export default defineComponent({
  name: "WarehouseAdminWarnings",
  setup() {
    const store = useStore(storeKey);

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
  border-radius: 20px;
  padding: 18px;
  border: 1px solid rgba(148, 197, 255, 0.28);
  background: linear-gradient(
    180deg,
    rgba(225, 237, 253, 0.97),
    rgba(205, 223, 247, 0.98)
  );
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
  color: #16385d;
}

.panel-head span {
  color: #587398;
  font-size: 12px;
}

.filter-pills {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.filter-pills span {
  padding: 9px 12px;
  border-radius: 999px;
  border: 1px solid rgba(176, 212, 255, 0.3);
  background: rgba(219, 232, 250, 0.86);
  color: #49658b;
}

.filter-pills .active {
  color: #12375f;
  border-color: rgba(103, 170, 255, 0.34);
  background: linear-gradient(
    180deg,
    rgba(168, 202, 251, 0.98),
    rgba(145, 184, 240, 0.96)
  );
}

.summary-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
  margin-bottom: 16px;
}

.summary-grid article,
.warning-row {
  padding: 16px;
  border-radius: 16px;
  border: 1px solid rgba(176, 212, 255, 0.3);
  background: rgba(217, 230, 248, 0.84);
}

.summary-grid small,
.summary-grid span,
.warning-row span {
  color: #597397;
}

.summary-grid strong {
  display: block;
  margin: 6px 0;
  font-size: 34px;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
}

.summary-grid article:nth-child(1) strong {
  color: #5db1ff;
}

.summary-grid article:nth-child(2) strong {
  color: #f07287;
}

.summary-grid article:nth-child(3) strong {
  color: #69d7ff;
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
  color: #173a60;
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
  color: #996100;
  background: rgba(160, 121, 53, 0.24);
}

.warning-row i.danger {
  color: #c63b57;
  background: rgba(154, 65, 88, 0.28);
}

.warning-row i.normal {
  color: #0f82bf;
  background: rgba(59, 136, 204, 0.24);
}

@media (max-width: 1100px) {
  .panel-head,
  .warning-row,
  .summary-grid {
    grid-template-columns: 1fr;
  }
}
</style>
