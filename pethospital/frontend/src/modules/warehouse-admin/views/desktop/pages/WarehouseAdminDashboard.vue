<template>
  <section class="page">
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
        label="待处理预警"
        :value="warningCount"
        hint="临期与低库存"
        tone="warning"
      />
    </div>

    <div class="workbench">
      <section class="dashboard-panel">
        <div class="section-title">
          <h3>库存列表</h3>
          <span>可批量勾选</span>
        </div>

        <div class="toolbar">
          <input
            v-model.trim="keyword"
            type="text"
            placeholder="按名称 / ID 查询"
          />
          <div class="chips">
            <button
              v-for="type in typeFilters"
              :key="type"
              :class="{ active: activeType === type }"
              @click="activeType = type"
            >
              {{ type }}
            </button>
          </div>
        </div>

        <div class="sort-strip">
          <span
            v-for="option in sortOptions"
            :key="option.key"
            :class="{ active: sortKey === option.key }"
            @click="sortKey = option.key"
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
                @change="toggleSelectAll"
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

          <div v-for="item in filteredItems" :key="item.id" class="grid-row">
            <span class="check-cell">
              <input
                :checked="isSelected(item.id)"
                type="checkbox"
                @change="toggleSelected(item.id)"
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
              <button class="mini-action" @click="openEdit(item)">
                详情 / 编辑
              </button>
            </span>
          </div>
        </div>
      </section>

      <div class="side-stack">
        <section class="side-panel">
          <div class="section-title">
            <h3>编辑抽屉</h3>
            <span>自动计算总价</span>
          </div>

          <div class="form-grid">
            <input
              v-model="editForm.item_name"
              type="text"
              :placeholder="editingItem ? '物品名称' : '请选择左侧记录后编辑'"
            />
            <input
              v-model="editForm.item_type"
              type="text"
              :placeholder="editingItem ? '物品类型' : '物品类型'"
            />
            <input
              v-model.number="editForm.item_number"
              type="number"
              min="1"
              :placeholder="editingItem ? '数量' : '数量'"
            />
            <input
              v-model.number="editForm.item_price"
              type="number"
              min="0.01"
              step="0.01"
              :placeholder="editingItem ? '单价' : '单价'"
            />
            <input
              type="text"
              :value="editSummaryTotal"
              readonly
              placeholder="总价"
            />
            <input
              type="text"
              :value="editingItem ? `SKU / ${editingItem.id}` : ''"
              readonly
              placeholder="SKU / 批次号"
            />
            <input
              v-model="editForm.item_productiondate"
              type="date"
              placeholder="生产日期"
            />
            <input
              v-model="editForm.item_expirationdate"
              type="date"
              placeholder="到期日期"
            />
          </div>

          <div class="meta-strip">
            <span>{{
              editingItem
                ? "当前库存 " + editingItem.item_number
                : "当前未选择物品"
            }}</span>
            <span>{{
              editingItem ? "修改后 " + editForm.item_number : "待编辑"
            }}</span>
            <span>总价自动更新</span>
          </div>

          <div class="panel-actions">
            <button class="ghost" @click="openCreatePreset">填入示例</button>
            <button @click="saveEdit">修改入库</button>
          </div>
        </section>

        <section class="delete-panel">
          <div class="section-title">
            <h3>删除确认</h3>
            <span>危险操作</span>
          </div>

          <template v-if="deletingItem">
            <p class="delete-title">即将删除 {{ deletingItem.item_name }}</p>
            <small
              >ID {{ deletingItem.id }} ·
              删除后将从库存列表和统计面板中移除</small
            >
            <input
              v-model.trim="deleteConfirmText"
              type="text"
              :placeholder="`输入 ${deletingItem.item_name} 确认删除`"
            />
            <button
              class="danger-btn"
              :disabled="deleteConfirmText !== deletingItem.item_name"
              @click="confirmDelete"
            >
              确认删除
            </button>
          </template>
          <template v-else>
            <p class="delete-title">删除候选物品</p>
            <small>先在左侧库存列表中选择一条记录，再进入删除确认。</small>
            <input type="text" placeholder="待选择物品" readonly />
            <button class="danger-btn muted" disabled>确认删除</button>
          </template>
        </section>
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, reactive, ref } from "vue";
import WarehouseStatCard from "@/modules/warehouse-admin/components/WarehouseStatCard.vue";
import { warehouseAdminApi } from "@/modules/warehouse-admin/api/warehouseAdminApi";
import {
  WarehouseCreatePayload,
  WarehouseItem,
} from "@/modules/warehouse-admin/api/types";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

export default defineComponent({
  name: "WarehouseAdminInventory",
  components: { WarehouseStatCard },
  setup() {
    const store = useStore(storeKey);

    /**
     * 创建一份默认表单，用于新增和编辑共用。
     */
    const createEmptyForm = (): WarehouseCreatePayload => ({
      item_name: "",
      item_type: "",
      item_productiondate: "",
      item_expirationdate: "",
      item_price: 0,
      item_number: 1,
    });

    const items = computed(() => store.state.warehouseAdmin.items);
    const keyword = ref("");
    const activeType = ref("全部");
    const sortKey = ref<"name" | "stock" | "price" | "total" | "expiry">(
      "name"
    );
    const editingItem = ref<WarehouseItem | null>(null);
    const deletingItem = ref<WarehouseItem | null>(null);
    const selectedIds = ref<number[]>([]);
    const deleteConfirmText = ref("");
    const editForm = reactive<WarehouseCreatePayload>(createEmptyForm());

    const typeFilters = ["全部", "药品", "耗材", "营养品"];
    const sortOptions = [
      { key: "name", label: "名称" },
      { key: "stock", label: "库存" },
      { key: "price", label: "单价" },
      { key: "total", label: "总价" },
      { key: "expiry", label: "到期日期" },
    ] as const;

    /**
     * 优先复用库存缓存，只有首次进入、过期或脏数据时才重拉。
     */
    const loadItems = async () => {
      await store.dispatch("warehouseAdmin/ensureItems");
    };

    const filteredItems = computed(() => {
      let rows = [...items.value];

      if (activeType.value !== "全部") {
        rows = rows.filter((item) => item.item_type === activeType.value);
      }

      if (keyword.value) {
        const lower = keyword.value.toLowerCase();
        rows = rows.filter(
          (item) =>
            item.item_name.toLowerCase().includes(lower) ||
            String(item.id).includes(lower)
        );
      }

      rows.sort((a, b) => {
        switch (sortKey.value) {
          case "stock":
            return b.item_number - a.item_number;
          case "price":
            return b.item_price - a.item_price;
          case "total":
            return b.item_totalprice - a.item_totalprice;
          case "expiry":
            return a.item_expirationdate.localeCompare(b.item_expirationdate);
          default:
            return a.item_name.localeCompare(b.item_name, "zh-CN");
        }
      });

      return rows;
    });

    const totalValue = computed(() =>
      items.value.reduce((sum, item) => sum + item.item_totalprice, 0)
    );

    const warningCount = computed(
      () =>
        items.value.filter(
          (item) =>
            item.item_number < 10 ||
            (item.days_until_expire !== null && item.days_until_expire <= 7)
        ).length
    );

    const computedEditTotal = computed(
      () => Number(editForm.item_number) * Number(editForm.item_price)
    );

    const editSummaryTotal = computed(() => {
      if (
        !editForm.item_name &&
        !editForm.item_type &&
        !editForm.item_productiondate &&
        !editForm.item_expirationdate &&
        Number(editForm.item_price) === 0 &&
        Number(editForm.item_number) === 1
      ) {
        return "";
      }

      return `¥ ${computedEditTotal.value.toFixed(2)}`;
    });

    const allVisibleSelected = computed(
      () =>
        filteredItems.value.length > 0 &&
        filteredItems.value.every((item) => selectedIds.value.includes(item.id))
    );

    const statusLabel = (item: WarehouseItem) => {
      if (item.days_until_expire !== null && item.days_until_expire <= 7) {
        return "临期";
      }
      if (item.item_number < 10) {
        return "低库存";
      }
      return "正常";
    };

    const statusTone = (item: WarehouseItem) => {
      if (item.days_until_expire !== null && item.days_until_expire <= 7) {
        return "warn";
      }
      if (item.item_number < 10) {
        return "danger";
      }
      return "ok";
    };

    const openEdit = (item: WarehouseItem) => {
      editingItem.value = item;
      deletingItem.value = item;
      deleteConfirmText.value = "";
      Object.assign(editForm, {
        item_name: item.item_name,
        item_type: item.item_type,
        item_productiondate: item.item_productiondate,
        item_expirationdate: item.item_expirationdate,
        item_price: item.item_price,
        item_number: item.item_number,
      });
    };

    const openCreatePreset = () => {
      editingItem.value = null;
      deletingItem.value = null;
      Object.assign(editForm, {
        item_name: "宠物补液套装",
        item_type: "耗材",
        item_productiondate: "2026-03-10",
        item_expirationdate: "2027-03-10",
        item_price: 68,
        item_number: 24,
      });
    };

    const toggleSelected = (itemId: number) => {
      if (selectedIds.value.includes(itemId)) {
        selectedIds.value = selectedIds.value.filter((id) => id !== itemId);
        return;
      }

      selectedIds.value = [...selectedIds.value, itemId];
    };

    const isSelected = (itemId: number) => selectedIds.value.includes(itemId);

    const toggleSelectAll = () => {
      if (allVisibleSelected.value) {
        const visibleIds = filteredItems.value.map((item) => item.id);
        selectedIds.value = selectedIds.value.filter(
          (id) => !visibleIds.includes(id)
        );
        return;
      }

      const merged = new Set(selectedIds.value);
      filteredItems.value.forEach((item) => merged.add(item.id));
      selectedIds.value = [...merged];
    };

    /**
     * 新增或修改库存后，刷新缓存并追加一条会话内操作流。
     */
    const saveEdit = async () => {
      try {
        const isEditing = Boolean(editingItem.value);
        if (editingItem.value) {
          await warehouseAdminApi.updateItem(editingItem.value.id, {
            ...editForm,
          });
        } else {
          await warehouseAdminApi.createItem({ ...editForm });
        }
        store.commit("warehouseAdmin/markItemsDirty");
        store.commit("warehouseAdmin/appendOperationLog", {
          time: new Date().toTimeString().slice(0, 5),
          title: isEditing
            ? `更新库存 · ${editForm.item_name}`
            : `新增物品 · ${editForm.item_name}`,
          description: `数量 ${editForm.item_number}，单价 ¥${Number(
            editForm.item_price
          ).toFixed(2)}。`,
          tag: isEditing ? "Update" : "Create",
        });
        await store.dispatch("warehouseAdmin/refreshItems");
      } catch {
        // 设计预览场景允许静默失败。
      }
    };

    /**
     * 删除库存后同步刷新缓存，并把本次操作写入会话内操作流。
     */
    const confirmDelete = async () => {
      if (!deletingItem.value) return;

      try {
        await warehouseAdminApi.deleteItem(deletingItem.value.id);
        store.commit("warehouseAdmin/markItemsDirty");
        store.commit("warehouseAdmin/appendOperationLog", {
          time: new Date().toTimeString().slice(0, 5),
          title: `删除物品 · ${deletingItem.value.item_name}`,
          description: `ID ${deletingItem.value.id} 已从库存列表中移除。`,
          tag: "Delete",
        });
        await store.dispatch("warehouseAdmin/refreshItems");
      } catch {
        // 设计预览场景允许静默失败。
      }
      selectedIds.value = selectedIds.value.filter(
        (id) => id !== deletingItem.value?.id
      );
      deletingItem.value = null;
      deleteConfirmText.value = "";
    };

    const currency = (value: number) => `¥ ${value.toLocaleString("zh-CN")}`;

    onMounted(() => {
      // 页面首次进入时优先复用仓库缓存，只有过期或脏数据时才会重拉。
      void loadItems();
      void store.dispatch("warehouseAdmin/ensureLogs");
    });

    return {
      items,
      keyword,
      activeType,
      sortKey,
      typeFilters,
      sortOptions,
      filteredItems,
      allVisibleSelected,
      editingItem,
      deletingItem,
      selectedIds,
      deleteConfirmText,
      editForm,
      computedEditTotal,
      editSummaryTotal,
      totalValue,
      warningCount,
      isSelected,
      openEdit,
      openCreatePreset,
      saveEdit,
      confirmDelete,
      toggleSelected,
      toggleSelectAll,
      statusLabel,
      statusTone,
      currency,
    };
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
}

.stats-row {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
  width: 100%;
}

.workbench {
  display: grid;
  grid-template-columns: minmax(0, 1.65fr) 420px;
  gap: 14px;
  flex: 1;
  min-height: 0;
  width: 100%;
}

.dashboard-panel,
.side-panel,
.delete-panel {
  border-radius: 20px;
  border: 1px solid rgba(108, 154, 255, 0.28);
  background: linear-gradient(
    180deg,
    rgba(34, 60, 116, 0.96),
    rgba(23, 45, 96, 0.98)
  );
  box-shadow: inset 0 0 0 1px rgba(141, 190, 255, 0.08),
    0 18px 34px rgba(34, 72, 145, 0.16);
}

.dashboard-panel {
  padding: 16px 16px 18px;
}

.side-stack {
  display: grid;
  gap: 14px;
}

.side-panel,
.delete-panel {
  padding: 16px;
}

.section-title {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 14px;
}

.section-title h3,
.delete-title {
  margin: 0;
  font-size: 16px;
  color: #edf4ff;
}

.section-title span,
.delete-panel small {
  color: #9fb4d4;
  font-size: 12px;
}

.toolbar {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 10px;
  margin-bottom: 12px;
}

.toolbar input,
.side-panel input,
.delete-panel input {
  min-width: 0;
  width: 80%;
  border: 1px solid rgba(120, 168, 255, 0.2);
  border-radius: 12px;
  padding: 12px 14px;
  background: rgba(255, 255, 255, 0.08);
  color: #d9e7fb;
  outline: none;
}

.toolbar input::placeholder,
.side-panel input::placeholder,
.delete-panel input::placeholder {
  color: #8da4c8;
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
  border-radius: 999px;
  border: 1px solid rgba(104, 145, 255, 0.34);
  background: linear-gradient(
    180deg,
    rgba(78, 116, 214, 0.46),
    rgba(55, 87, 176, 0.46)
  );
  color: #c4d8fb;
  font-size: 12px;
  cursor: pointer;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.06);
}

.chips button {
  padding: 10px 12px;
}

.chips button.active,
.sort-strip span.active {
  color: #f3f8ff;
  border-color: rgba(113, 160, 255, 0.58);
  background: linear-gradient(
    180deg,
    rgba(76, 122, 255, 0.96),
    rgba(51, 95, 224, 0.98)
  );
  box-shadow: 0 10px 18px rgba(49, 91, 207, 0.36);
}

.sort-strip {
  margin-bottom: 12px;
  padding: 10px;
  border-radius: 14px;
  background: rgba(255, 255, 255, 0.08);
}

.sort-strip span {
  padding: 9px 14px;
}

.grid-table {
  border: 1px solid rgba(120, 168, 255, 0.16);
  border-radius: 16px;
  overflow: hidden;
  background: rgba(12, 27, 57, 0.34);
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
  color: #9ab0cf;
  font-size: 12px;
  border-bottom: 1px solid rgba(117, 184, 255, 0.08);
}

.grid-row {
  color: #e8f1ff;
  font-size: 13px;
  border-top: 1px solid rgba(117, 184, 255, 0.05);
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
  accent-color: #5d84ff;
  cursor: pointer;
}

.name-cell b {
  display: block;
}

.name-cell small {
  color: #a4b8d5;
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
  color: #75dfbe;
  background: rgba(56, 143, 117, 0.2);
}

.status.warn {
  color: #edbb67;
  background: rgba(124, 90, 32, 0.24);
}

.status.danger {
  color: #f07287;
  background: rgba(111, 39, 56, 0.28);
}

.action-cell {
  display: flex;
}

.mini-action,
.panel-actions button,
.danger-btn {
  width: 100%;
  border: 1px solid rgba(117, 166, 255, 0.24);
  border-radius: 12px;
  padding: 10px 12px;
  background: rgba(255, 255, 255, 0.08);
  color: #dbe8fb;
  cursor: pointer;
}

.panel-actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.panel-actions .ghost {
  background: rgba(255, 255, 255, 0.06);
}

.panel-actions button:last-child {
  background: linear-gradient(135deg, #6a86ff, #4169e1);
  color: #f7fbff;
  font-weight: 700;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.meta-strip {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
  margin: 14px 0;
}

.meta-strip span {
  padding: 8px 12px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.08);
  color: #aac0de;
  font-size: 12px;
}

.delete-panel {
  background: linear-gradient(
      180deg,
      rgba(8, 22, 37, 0.98),
      rgba(7, 16, 31, 0.98)
    ),
    linear-gradient(180deg, rgba(111, 32, 52, 0.18), rgba(111, 32, 52, 0));
}

.delete-panel small {
  display: block;
  margin: 6px 0 12px;
}

.danger-btn {
  margin-top: 12px;
  color: #ff9dac;
  border-color: rgba(194, 77, 102, 0.34);
  background: linear-gradient(
    180deg,
    rgba(111, 42, 60, 0.38),
    rgba(125, 43, 67, 0.46)
  );
}

.danger-btn:disabled {
  opacity: 0.62;
  cursor: not-allowed;
}

.danger-btn.muted {
  color: #ba8793;
}

@media (max-width: 1280px) {
  .workbench {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 1080px) {
  .stats-row {
    grid-template-columns: 1fr;
  }

  .toolbar {
    grid-template-columns: 1fr;
  }

  .form-grid {
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
  .page {
    gap: 12px;
  }

  .dashboard-panel,
  .side-panel,
  .delete-panel {
    border-radius: 16px;
  }

  .dashboard-panel,
  .side-panel,
  .delete-panel,
  .sort-strip {
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
    color: #9ab0cf;
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
  .mini-action,
  .panel-actions {
    width: 100%;
  }

  .panel-actions {
    grid-template-columns: 1fr;
  }
}
</style>
