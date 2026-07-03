<!-- Renders the warehouse inventory ledger table, filters, sorting, selection, and pager. -->
<template>
  <section class="ledger-panel">
    <div class="section-title">
      <div>
        <h3>库存列表</h3>
        <span>共 {{ total }} 条记录</span>
      </div>
      <b>{{ selectedCount }} 项已选</b>
    </div>

    <div class="toolbar">
      <input
        :value="keywordInput"
        type="text"
        placeholder="按物品名称查询"
        @input="updateKeywordInput"
        @keyup.enter="$emit('apply-search')"
      />
      <div class="chips">
        <button
          v-for="type in typeFilters"
          :key="type"
          :class="{ active: activeType === type }"
          @click="$emit('update:activeType', type)"
        >
          {{ type }}
        </button>
      </div>
    </div>

    <div class="sort-strip">
      <small>排序</small>
      <span
        v-for="option in sortOptions"
        :key="option.key"
        :class="{ active: sortKey === option.key }"
        @click="$emit('update:sortKey', option.key)"
      >
        {{ option.label }}
      </span>
    </div>

    <div class="grid-table">
      <div class="grid-head">
        <span class="head-check">
          <input
            type="checkbox"
            :checked="allVisibleSelected"
            @change="$emit('toggle-select-all')"
          />
        </span>
        <span>名称</span>
        <span>类型</span>
        <span>库存</span>
        <span>单价</span>
        <span>总价</span>
        <span>状态</span>
        <span>到期日期</span>
        <span>操作</span>
      </div>

      <div v-for="item in items" :key="item.id" class="grid-row">
        <span class="check-cell">
          <input
            :checked="isSelected(item.id)"
            type="checkbox"
            @change="$emit('toggle-selected', item.id)"
          />
        </span>
        <span class="name-cell">
          <b>{{ item.item_name }}</b>
          <small>ID {{ item.id }}</small>
        </span>
        <span>{{ item.item_type }}</span>
        <span>{{ item.item_number }}</span>
        <span>{{ item.item_price.toFixed(2) }}</span>
        <span>{{ item.item_totalprice.toFixed(2) }}</span>
        <span>
          <i class="status" :class="statusTone(item)">{{
            statusLabel(item)
          }}</i>
        </span>
        <span>{{ item.item_expirationdate }}</span>
        <span class="action-cell">
          <button class="mini-action" @click="$emit('open-edit', item)">
            详情 / 编辑
          </button>
        </span>
      </div>

      <div
        v-for="index in placeholderRows"
        :key="`placeholder-${index}`"
        class="grid-row grid-row--placeholder"
        aria-hidden="true"
      >
        <span></span>
        <span></span>
        <span></span>
        <span></span>
        <span></span>
        <span></span>
        <span></span>
        <span></span>
        <span></span>
      </div>
    </div>

    <AppPager
      :page="page"
      :total-pages="totalPages"
      @update:page="$emit('update:page', $event)"
    />
  </section>
</template>

<script lang="ts">
import { defineComponent, PropType } from "vue";
import AppPager from "@/shared/components/AppPager.vue";
import { WarehouseItem } from "@/modules/warehouse-admin/api/types";
import { WarehouseInventorySortKey } from "@/modules/warehouse-admin/composables/useWarehouseInventoryDashboard";

type SortOption = {
  key: WarehouseInventorySortKey;
  label: string;
};

export default defineComponent({
  name: "WarehouseInventoryLedger",
  components: { AppPager },
  props: {
    total: {
      type: Number,
      required: true,
    },
    selectedCount: {
      type: Number,
      required: true,
    },
    keywordInput: {
      type: String,
      required: true,
    },
    activeType: {
      type: String,
      required: true,
    },
    sortKey: {
      type: String as PropType<WarehouseInventorySortKey>,
      required: true,
    },
    typeFilters: {
      type: Array as PropType<string[]>,
      required: true,
    },
    sortOptions: {
      type: Array as PropType<ReadonlyArray<SortOption>>,
      required: true,
    },
    allVisibleSelected: {
      type: Boolean,
      required: true,
    },
    items: {
      type: Array as PropType<WarehouseItem[]>,
      required: true,
    },
    placeholderRows: {
      type: Array as PropType<number[]>,
      required: true,
    },
    page: {
      type: Number,
      required: true,
    },
    totalPages: {
      type: Number,
      required: true,
    },
    isSelected: {
      type: Function as PropType<(_itemId: number) => boolean>,
      required: true,
    },
    statusLabel: {
      type: Function as PropType<(_item: WarehouseItem) => string>,
      required: true,
    },
    statusTone: {
      type: Function as PropType<(_item: WarehouseItem) => string>,
      required: true,
    },
  },
  emits: [
    "update:keywordInput",
    "update:activeType",
    "update:sortKey",
    "update:page",
    "apply-search",
    "toggle-select-all",
    "toggle-selected",
    "open-edit",
  ],
  methods: {
    updateKeywordInput(event: Event) {
      const target = event.target as HTMLInputElement;
      this.$emit("update:keywordInput", target.value.trim());
    },
  },
});
</script>

<style scoped>
.ledger-panel {
  border-radius: 12px;
  border: 1px solid #e7e9ee;
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(16, 24, 40, 0.06);
  min-width: 0;
  padding: 16px;
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

.toolbar {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 10px;
  margin-bottom: 12px;
}

.toolbar input {
  min-width: 0;
  width: 100%;
  box-sizing: border-box;
  border: 1px solid #e7e9ee;
  border-radius: 8px;
  padding: 11px 12px;
  background: #ffffff;
  color: #0f172a;
  outline: none;
}

.toolbar input::placeholder {
  color: #94a3b8;
}

.chips,
.sort-strip {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.chips button,
.sort-strip span {
  border-radius: 8px;
  border: 1px solid #e7e9ee;
  background: #ffffff;
  color: #64748b;
  font-size: 12px;
  cursor: pointer;
}

.chips button {
  padding: 10px 12px;
}

.chips button.active,
.sort-strip span.active {
  color: #ffffff;
  border-color: #4f46e5;
  background: #4f46e5;
}

.sort-strip {
  margin-bottom: 12px;
  padding: 10px;
  border-radius: 10px;
  background: #f1f2f5;
}

.sort-strip small {
  color: #64748b;
  font-size: 12px;
}

.sort-strip span {
  padding: 9px 14px;
}

.grid-table {
  border: 1px solid #e7e9ee;
  border-radius: 10px;
  overflow: hidden;
  background: #ffffff;
}

.grid-head,
.grid-row {
  display: grid;
  grid-template-columns: 24px 1.7fr 1fr 0.7fr 0.8fr 0.9fr 0.9fr 1fr 1fr;
  gap: 12px;
  align-items: center;
  padding: 12px 14px;
}

.grid-head {
  color: #64748b;
  font-size: 12px;
  border-bottom: 1px solid #e7e9ee;
  background: #f1f2f5;
}

.grid-row {
  color: #0f172a;
  font-size: 13px;
  border-top: 1px solid #f1f2f4;
  transition: background 0.18s ease;
}

.grid-row:hover {
  background: #fafbff;
}

.grid-row--placeholder {
  min-height: 52px;
  color: transparent;
  pointer-events: none;
}

.grid-row--placeholder:hover {
  background: #ffffff;
}

.grid-row:first-of-type {
  border-top: 0;
}

.head-check,
.check-cell {
  display: flex;
  align-items: center;
  justify-content: center;
}

.head-check input,
.check-cell input {
  width: 16px;
  height: 16px;
  margin: 0;
  accent-color: #4f46e5;
  cursor: pointer;
}

.name-cell b {
  display: block;
}

.name-cell small {
  color: #94a3b8;
}

.status {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 56px;
  padding: 5px 10px;
  border-radius: 999px;
  font-style: normal;
  font-size: 11px;
}

.status.ok {
  color: #059669;
  background: rgba(236, 253, 245, 0.9);
}

.status.warn {
  color: #b45309;
  background: rgba(255, 251, 235, 0.9);
}

.status.danger {
  color: #dc2626;
  background: rgba(254, 242, 242, 0.95);
}

.action-cell {
  display: flex;
}

.mini-action {
  width: 100%;
  border: 1px solid #e7e9ee;
  border-radius: 8px;
  padding: 10px 12px;
  background: #ffffff;
  color: #4f46e5;
  font-weight: 700;
  cursor: pointer;
}

@media (max-width: 1080px) {
  .toolbar {
    grid-template-columns: 1fr;
  }

  .grid-table {
    overflow-x: auto;
  }

  .grid-head,
  .grid-row {
    min-width: 720px;
  }
}

@media (max-width: 720px) {
  .ledger-panel {
    border-radius: 12px;
    padding-left: 12px;
    padding-right: 12px;
  }

  .section-title {
    flex-direction: column;
    align-items: flex-start;
    gap: 6px;
  }

  .chips {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    width: 100%;
  }

  .chips button {
    width: 100%;
  }

  .sort-strip {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    padding-left: 12px;
    padding-right: 12px;
  }

  .sort-strip span {
    text-align: center;
  }

  .grid-table {
    overflow: visible;
    border-radius: 14px;
  }

  .grid-head {
    display: none;
  }

  .grid-row {
    min-width: 0;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 10px 12px;
    padding: 14px 12px;
  }

  .grid-row > span {
    display: grid;
    gap: 4px;
    min-width: 0;
  }

  .grid-row > span::before {
    font-size: 11px;
    color: #94a3b8;
  }

  .grid-row > span:nth-child(1) {
    display: none;
  }

  .grid-row > span:nth-child(2),
  .grid-row > span:nth-child(9) {
    grid-column: 1 / -1;
  }

  .grid-row > span:nth-child(2)::before {
    content: "名称";
  }

  .grid-row > span:nth-child(3)::before {
    content: "类型";
  }

  .grid-row > span:nth-child(4)::before {
    content: "库存";
  }

  .grid-row > span:nth-child(5)::before {
    content: "单价";
  }

  .grid-row > span:nth-child(6)::before {
    content: "总价";
  }

  .grid-row > span:nth-child(7)::before {
    content: "状态";
  }

  .grid-row > span:nth-child(8)::before {
    content: "到期日期";
  }

  .grid-row > span:nth-child(9)::before {
    content: "操作";
  }

  .action-cell,
  .mini-action {
    width: 100%;
  }
}
</style>
