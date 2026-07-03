<template>
  <section class="page">
    <header class="command-head">
      <div>
        <p>库存工作台</p>
        <h2>库存台账</h2>
        <span>以库存列表为核心，快速完成查询、预警处理和库存动作</span>
      </div>
      <button type="button" @click="openCreatePreset">新增入库</button>
    </header>

    <p
      v-if="statusMessage"
      class="status-message"
      :class="`status-message--${statusType}`"
    >
      {{ statusMessage }}
    </p>

    <div class="stats-row">
      <WarehouseStatCard
        label="在库品项"
        :value="items.length"
        hint="实时同步"
      />
      <WarehouseStatCard
        label="库存总值"
        :value="currency(totalValue)"
        hint="自动汇总"
      />
      <WarehouseStatCard
        label="低库存"
        :value="lowStockCount"
        hint="少于 10 件"
        tone="danger"
      />
      <WarehouseStatCard
        label="临期物品"
        :value="expiringCount"
        hint="7 天内到期"
        tone="warning"
      />
    </div>

    <WarehouseInventoryLedger
      v-model:keyword-input="keywordInput"
      v-model:active-type="activeType"
      v-model:sort-key="sortKey"
      v-model:page="page"
      :total="total"
      :selected-count="selectedIds.length"
      :type-filters="typeFilters"
      :sort-options="sortOptions"
      :all-visible-selected="allVisibleSelected"
      :items="filteredItems"
      :placeholder-rows="placeholderRows"
      :total-pages="totalPages"
      :is-selected="isSelected"
      :status-label="statusLabel"
      :status-tone="statusTone"
      @apply-search="applySearch"
      @toggle-select-all="toggleSelectAll"
      @toggle-selected="toggleSelected"
      @open-edit="openEdit"
    />

    <WarehouseWarningPanel
      :warning-count="warningCount"
      :warning-items="warningItems"
      :warning-reason="warningReason"
      @open-edit="openEdit"
    />

    <WarehouseInventoryDrawer
      v-if="drawerOpen"
      v-model:movement-quantity="movementQuantity"
      v-model:delete-confirm-text="deleteConfirmText"
      :editing-item="editingItem"
      :deleting-item="deletingItem"
      :edit-form="editForm"
      :edit-summary-total="editSummaryTotal"
      @update-edit-form-field="updateEditFormField"
      @close="closeDrawer"
      @save-edit="saveEdit"
      @stock-in="submitStockIn"
      @stock-out="submitStockOut"
      @confirm-delete="confirmDelete"
    />
  </section>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import WarehouseStatCard from "@/modules/warehouse-admin/components/WarehouseStatCard.vue";
import WarehouseInventoryLedger from "@/modules/warehouse-admin/components/WarehouseInventoryLedger.vue";
import WarehouseWarningPanel from "@/modules/warehouse-admin/components/WarehouseWarningPanel.vue";
import WarehouseInventoryDrawer from "@/modules/warehouse-admin/components/WarehouseInventoryDrawer.vue";
import { useWarehouseInventoryDashboard } from "@/modules/warehouse-admin/composables/useWarehouseInventoryDashboard";

export default defineComponent({
  name: "WarehouseAdminInventory",
  components: {
    WarehouseStatCard,
    WarehouseInventoryLedger,
    WarehouseWarningPanel,
    WarehouseInventoryDrawer,
  },
  setup() {
    return useWarehouseInventoryDashboard();
  },
});
</script>

<style scoped>
.page {
  display: flex;
  flex-direction: column;
  width: 100%;
  min-height: 100%;
  box-sizing: border-box;
  gap: 14px;
  color: #0f172a;
}

.command-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  border: 1px solid #e7e9ee;
  border-radius: 14px;
  padding: 16px 18px;
  background: #ffffff;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

.command-head p,
.command-head h2,
.command-head span {
  margin: 0;
}

.command-head p {
  color: #64748b;
  font-size: 11px;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.command-head h2 {
  margin: 4px 0;
  font-size: 26px;
  line-height: 1.15;
  color: #0f172a;
}

.command-head span {
  color: #64748b;
  font-size: 13px;
}

.command-head button {
  flex: 0 0 auto;
  border: 0;
  border-radius: 8px;
  padding: 11px 16px;
  background: #4f46e5;
  color: #ffffff;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 10px 20px rgba(79, 70, 229, 0.18);
}

.status-message {
  margin: 0;
  border-radius: 14px;
  padding: 11px 14px;
  font-size: 13px;
}

.status-message--info {
  color: #047857;
  background: rgba(236, 253, 245, 0.9);
  border: 1px solid rgba(16, 185, 129, 0.14);
}

.status-message--error {
  color: #b91c1c;
  background: rgba(254, 242, 242, 0.92);
  border: 1px solid rgba(220, 38, 38, 0.16);
}

.stats-row {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
  width: 100%;
}

@media (max-width: 1080px) {
  .stats-row {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 720px) {
  .page {
    gap: 12px;
  }
}

@media (max-width: 640px) {
  .command-head {
    align-items: flex-start;
    flex-direction: column;
  }

  .command-head button {
    width: 100%;
  }

  .stats-row {
    grid-template-columns: 1fr;
  }
}
</style>
