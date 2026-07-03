<!-- Displays low-stock and expiring warehouse items that need attention. -->
<template>
  <section class="warning-panel">
    <div class="section-title">
      <div>
        <h3>预警队列</h3>
        <span>低库存与临期优先处理</span>
      </div>
      <b>{{ warningCount }} 项</b>
    </div>

    <div v-if="warningItems.length" class="warning-list">
      <button
        v-for="item in warningItems"
        :key="item.id"
        type="button"
        class="warning-row"
        @click="$emit('open-edit', item)"
      >
        <span>
          <strong>{{ item.item_name }}</strong>
          <small>{{ warningReason(item) }}</small>
        </span>
        <em>{{ item.item_number }} 件</em>
      </button>
    </div>
    <div v-else class="empty-warning">暂无需要处理的库存预警</div>
  </section>
</template>

<script lang="ts">
import { defineComponent, PropType } from "vue";
import { WarehouseItem } from "@/modules/warehouse-admin/api/types";

export default defineComponent({
  name: "WarehouseWarningPanel",
  props: {
    warningCount: {
      type: Number,
      required: true,
    },
    warningItems: {
      type: Array as PropType<WarehouseItem[]>,
      required: true,
    },
    warningReason: {
      type: Function as PropType<(_item: WarehouseItem) => string>,
      required: true,
    },
  },
  emits: ["open-edit"],
});
</script>

<style scoped>
.warning-panel {
  border-radius: 12px;
  border: 1px solid #e7e9ee;
  padding: 14px;
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(16, 24, 40, 0.06);
}

.section-title {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 14px;
  gap: 12px;
}

.section-title h3 {
  margin: 0 0 4px;
  color: #0f172a;
  font-size: 16px;
}

.section-title b {
  flex: 0 0 auto;
  border-radius: 8px;
  padding: 6px 10px;
  background: #eef2ff;
  color: #4338ca;
  font-size: 12px;
}

.section-title span {
  color: #64748b;
  font-size: 12px;
}

.warning-list {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
}

.warning-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  width: 100%;
  border: 1px solid #fde68a;
  border-radius: 10px;
  padding: 10px 12px;
  background: #fffbeb;
  color: #0f172a;
  text-align: left;
  cursor: pointer;
}

.warning-row strong,
.warning-row small {
  display: block;
}

.warning-row small {
  margin-top: 3px;
  color: #b45309;
  font-size: 12px;
}

.warning-row em {
  flex: 0 0 auto;
  border-radius: 999px;
  padding: 6px 9px;
  background: rgba(255, 251, 235, 0.9);
  color: #b45309;
  font-size: 12px;
  font-style: normal;
  font-weight: 800;
}

.empty-warning {
  border: 1px dashed #e7e9ee;
  border-radius: 10px;
  padding: 18px;
  color: #94a3b8;
  text-align: center;
}

@media (max-width: 1280px) {
  .warning-list {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 720px) {
  .section-title {
    flex-direction: column;
    align-items: flex-start;
    gap: 6px;
  }
}

@media (max-width: 640px) {
  .warning-list {
    grid-template-columns: 1fr;
  }
}
</style>
