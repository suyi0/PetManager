<template>
  <section class="page">
    <header class="command-head">
      <div>
        <p>Warehouse Console</p>
        <h2>仓储调度台</h2>
        <span>库存入库、出库、预警和基础档案统一处理</span>
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

    <div class="workbench">
      <section class="dashboard-panel">
        <div class="section-title">
          <div>
            <h3>库存列表</h3>
            <span>共 {{ filteredItems.length }} 条记录</span>
          </div>
          <b>{{ selectedIds.length }} 项已选</b>
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
            <div>
              <h3>快速操作</h3>
              <span>{{
                editingItem ? editingItem.item_name : "未选择库存记录"
              }}</span>
            </div>
            <b>{{ editingItem ? "编辑中" : "新增模式" }}</b>
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

          <div class="movement-box">
            <input
              v-model.number="movementQuantity"
              type="number"
              min="1"
              placeholder="入库 / 出库数量"
            />
            <button
              class="ghost"
              :disabled="!editingItem || movementQuantity <= 0"
              @click="submitStockIn"
            >
              入库
            </button>
            <button
              class="ghost"
              :disabled="!editingItem || movementQuantity <= 0"
              @click="submitStockOut"
            >
              出库
            </button>
          </div>

          <div class="panel-actions">
            <button class="ghost" @click="openCreatePreset">新增模式</button>
            <button @click="saveEdit">
              {{ editingItem ? "保存修改" : "新增入库" }}
            </button>
          </div>
        </section>

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
              @click="openEdit(item)"
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

        <section class="delete-panel">
          <div class="section-title">
            <div>
              <h3>删除确认</h3>
              <span>危险操作</span>
            </div>
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
    const movementQuantity = ref(1);
    const statusMessage = ref("");
    const statusType = ref<"info" | "error">("info");
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

    const lowStockCount = computed(
      () => items.value.filter((item) => item.item_number < 10).length
    );

    const expiringCount = computed(
      () =>
        items.value.filter(
          (item) =>
            item.days_until_expire !== null && item.days_until_expire <= 7
        ).length
    );

    const warningItems = computed(() =>
      items.value
        .filter(
          (item) =>
            item.item_number < 10 ||
            (item.days_until_expire !== null && item.days_until_expire <= 7)
        )
        .sort((a, b) => {
          const aExpire = a.days_until_expire ?? Number.MAX_SAFE_INTEGER;
          const bExpire = b.days_until_expire ?? Number.MAX_SAFE_INTEGER;
          return aExpire - bExpire || a.item_number - b.item_number;
        })
        .slice(0, 6)
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

    const warningReason = (item: WarehouseItem) => {
      if (item.days_until_expire !== null && item.days_until_expire <= 7) {
        return `${item.days_until_expire} 天内到期`;
      }
      return "库存低于 10 件";
    };

    const openEdit = (item: WarehouseItem) => {
      editingItem.value = item;
      deletingItem.value = item;
      deleteConfirmText.value = "";
      movementQuantity.value = 1;
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
      movementQuantity.value = 1;
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

    const showStatus = (message: string, type: "info" | "error" = "info") => {
      statusMessage.value = message;
      statusType.value = type;
    };

    /**
     * 新增或修改库存后，刷新缓存并追加一条会话内操作流。
     */
    const saveEdit = async () => {
      try {
        if (editingItem.value) {
          await store.dispatch("warehouseAdmin/updateItem", {
            itemId: editingItem.value.id,
            patch: { ...editForm },
          });
        } else {
          await store.dispatch("warehouseAdmin/createItem", { ...editForm });
        }
        showStatus(editingItem.value ? "库存信息已更新" : "物品已新增入库");
      } catch (error) {
        showStatus(
          `保存失败：${String((error as Error).message || error)}`,
          "error"
        );
      }
    };

    const submitStockIn = async () => {
      if (!editingItem.value) return;

      try {
        await store.dispatch("warehouseAdmin/stockIn", {
          itemId: editingItem.value.id,
          quantity: Number(movementQuantity.value),
        });
        showStatus("入库操作已完成");
      } catch (error) {
        showStatus(
          `入库失败：${String((error as Error).message || error)}`,
          "error"
        );
      }
    };

    const submitStockOut = async () => {
      if (!editingItem.value) return;

      try {
        await store.dispatch("warehouseAdmin/stockOut", {
          itemId: editingItem.value.id,
          quantity: Number(movementQuantity.value),
        });
        showStatus("出库操作已完成");
      } catch (error) {
        showStatus(
          `出库失败：${String((error as Error).message || error)}`,
          "error"
        );
      }
    };

    /**
     * 删除库存后同步刷新缓存，并把本次操作写入会话内操作流。
     */
    const confirmDelete = async () => {
      if (!deletingItem.value) return;

      try {
        await store.dispatch(
          "warehouseAdmin/deleteItem",
          deletingItem.value.id
        );
        showStatus("物品已删除");
      } catch (error) {
        showStatus(
          `删除失败：${String((error as Error).message || error)}`,
          "error"
        );
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
      movementQuantity,
      statusMessage,
      statusType,
      editForm,
      computedEditTotal,
      editSummaryTotal,
      totalValue,
      warningCount,
      lowStockCount,
      expiringCount,
      warningItems,
      isSelected,
      openEdit,
      openCreatePreset,
      saveEdit,
      submitStockIn,
      submitStockOut,
      confirmDelete,
      toggleSelected,
      toggleSelectAll,
      statusLabel,
      statusTone,
      warningReason,
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
  gap: 16px;
  color: #173c45;
}

.command-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  border: 1px solid rgba(113, 166, 176, 0.2);
  border-radius: 20px;
  padding: 18px 20px;
  background: linear-gradient(
      135deg,
      rgba(255, 255, 255, 0.94),
      rgba(238, 250, 248, 0.88)
    ),
    linear-gradient(180deg, #f8fbf8, #e5f4f3);
  box-shadow: 0 18px 42px rgba(57, 102, 115, 0.1);
}

.command-head p,
.command-head h2,
.command-head span {
  margin: 0;
}

.command-head p {
  color: #6f8f92;
  font-size: 11px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.command-head h2 {
  margin: 4px 0;
  font-size: 28px;
  line-height: 1.15;
  color: #12383d;
}

.command-head span {
  color: #668288;
  font-size: 13px;
}

.command-head button {
  flex: 0 0 auto;
  border: 0;
  border-radius: 14px;
  padding: 12px 18px;
  background: linear-gradient(135deg, #1f7077, #6d5348);
  color: #ffffff;
  font-weight: 800;
  cursor: pointer;
  box-shadow: 0 14px 28px rgba(48, 93, 98, 0.22);
}

.status-message {
  margin: 0;
  border-radius: 14px;
  padding: 11px 14px;
  font-size: 13px;
}

.status-message--info {
  color: #1f684b;
  background: rgba(215, 242, 230, 0.9);
  border: 1px solid rgba(74, 146, 105, 0.14);
}

.status-message--error {
  color: #a23c4a;
  background: rgba(255, 229, 234, 0.92);
  border: 1px solid rgba(180, 65, 83, 0.16);
}

.stats-row {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
  width: 100%;
}

.workbench {
  display: grid;
  grid-template-columns: minmax(0, 1.7fr) 420px;
  gap: 14px;
  flex: 1;
  min-height: 0;
  width: 100%;
}

.dashboard-panel,
.side-panel,
.warning-panel,
.delete-panel {
  border-radius: 18px;
  border: 1px solid rgba(108, 151, 156, 0.2);
  background: rgba(250, 253, 250, 0.9);
  box-shadow: 0 18px 36px rgba(45, 82, 92, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.82);
}

.dashboard-panel {
  min-width: 0;
  padding: 18px;
}

.side-stack {
  display: grid;
  align-content: start;
  gap: 14px;
}

.side-panel,
.warning-panel,
.delete-panel {
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
  color: #163d42;
  font-size: 16px;
}

.section-title b {
  flex: 0 0 auto;
  border-radius: 999px;
  padding: 6px 10px;
  background: rgba(221, 241, 237, 0.9);
  color: #2d6f70;
  font-size: 12px;
}

.delete-title {
  margin: 0;
  font-size: 16px;
  color: #163d42;
}

.section-title span,
.delete-panel small {
  color: #6a8589;
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
  width: 100%;
  box-sizing: border-box;
  border: 1px solid rgba(91, 139, 143, 0.18);
  border-radius: 12px;
  padding: 12px 14px;
  background: rgba(255, 255, 255, 0.86);
  color: #153f45;
  outline: none;
}

.toolbar input::placeholder,
.side-panel input::placeholder,
.delete-panel input::placeholder {
  color: #90a7aa;
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
  border: 1px solid rgba(102, 150, 154, 0.22);
  background: rgba(248, 252, 249, 0.86);
  color: #526f75;
  font-size: 12px;
  cursor: pointer;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.66);
}

.chips button {
  padding: 10px 12px;
}

.chips button.active,
.sort-strip span.active {
  color: #ffffff;
  border-color: rgba(31, 112, 119, 0.38);
  background: linear-gradient(135deg, #1f7077, #426d69);
  box-shadow: 0 10px 18px rgba(49, 99, 105, 0.22);
}

.sort-strip {
  margin-bottom: 12px;
  padding: 10px;
  border-radius: 14px;
  background: rgba(234, 247, 244, 0.74);
}

.sort-strip span {
  padding: 9px 14px;
}

.grid-table {
  border: 1px solid rgba(101, 146, 150, 0.16);
  border-radius: 16px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.78);
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
  color: #668085;
  font-size: 12px;
  border-bottom: 1px solid rgba(107, 151, 154, 0.12);
  background: rgba(237, 248, 245, 0.84);
}

.grid-row {
  color: #173f45;
  font-size: 13px;
  border-top: 1px solid rgba(107, 151, 154, 0.1);
  transition: background 0.18s ease, transform 0.18s ease;
}

.grid-row:hover {
  background: rgba(238, 249, 246, 0.82);
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
  accent-color: #1f7077;
  cursor: pointer;
}

.name-cell b {
  display: block;
}

.name-cell small {
  color: #789093;
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
  color: #247b62;
  background: rgba(214, 242, 232, 0.9);
}

.status.warn {
  color: #9b6817;
  background: rgba(255, 236, 202, 0.9);
}

.status.danger {
  color: #b04455;
  background: rgba(255, 224, 229, 0.95);
}

.action-cell {
  display: flex;
}

.mini-action,
.panel-actions button,
.danger-btn {
  width: 100%;
  border: 1px solid rgba(91, 139, 143, 0.2);
  border-radius: 12px;
  padding: 10px 12px;
  background: rgba(255, 255, 255, 0.86);
  color: #24565c;
  font-weight: 700;
  cursor: pointer;
}

.panel-actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.panel-actions .ghost {
  background: rgba(238, 249, 246, 0.88);
}

.panel-actions button:last-child {
  border-color: rgba(31, 112, 119, 0.24);
  background: linear-gradient(135deg, #1f7077, #6d5348);
  color: #ffffff;
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
  background: rgba(234, 247, 244, 0.8);
  color: #5b777b;
  font-size: 12px;
}

.movement-box {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto auto;
  gap: 8px;
  margin: 10px 0 14px;
}

.movement-box button {
  border: 1px solid rgba(91, 139, 143, 0.18);
  border-radius: 12px;
  padding: 0 14px;
  background: #f8fcf9;
  color: #1d6067;
  font-weight: 800;
  cursor: pointer;
}

.movement-box button:disabled {
  color: #9dafb1;
  cursor: not-allowed;
  opacity: 0.68;
}

.warning-panel {
  background: linear-gradient(
      180deg,
      rgba(255, 252, 245, 0.94),
      rgba(250, 253, 250, 0.9)
    ),
    #ffffff;
}

.warning-list {
  display: grid;
  gap: 8px;
}

.warning-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  width: 100%;
  border: 1px solid rgba(166, 122, 70, 0.16);
  border-radius: 14px;
  padding: 10px 12px;
  background: rgba(255, 255, 255, 0.78);
  color: #173f45;
  text-align: left;
  cursor: pointer;
}

.warning-row strong,
.warning-row small {
  display: block;
}

.warning-row small {
  margin-top: 3px;
  color: #8a7060;
  font-size: 12px;
}

.warning-row em {
  flex: 0 0 auto;
  border-radius: 999px;
  padding: 6px 9px;
  background: rgba(255, 236, 202, 0.9);
  color: #9b6817;
  font-size: 12px;
  font-style: normal;
  font-weight: 800;
}

.empty-warning {
  border: 1px dashed rgba(97, 142, 146, 0.22);
  border-radius: 14px;
  padding: 18px;
  color: #789093;
  text-align: center;
}

.delete-panel {
  background: linear-gradient(
      180deg,
      rgba(255, 247, 249, 0.95),
      rgba(250, 253, 250, 0.9)
    ),
    #ffffff;
}

.delete-panel small {
  display: block;
  margin: 6px 0 12px;
}

.danger-btn {
  margin-top: 12px;
  color: #b04455;
  border-color: rgba(194, 77, 102, 0.24);
  background: linear-gradient(
    180deg,
    rgba(255, 236, 240, 0.86),
    rgba(255, 225, 231, 0.92)
  );
}

.danger-btn:disabled {
  opacity: 0.62;
  cursor: not-allowed;
}

.danger-btn.muted {
  color: #b7969e;
}

@media (max-width: 1280px) {
  .workbench {
    grid-template-columns: 1fr;
  }

  .side-stack {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }
}

@media (max-width: 1080px) {
  .stats-row {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .side-stack {
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
    color: #789093;
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

@media (max-width: 640px) {
  .command-head {
    align-items: flex-start;
    flex-direction: column;
  }

  .command-head button {
    width: 100%;
  }

  .stats-row,
  .movement-box {
    grid-template-columns: 1fr;
  }
}
</style>
