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

    <section class="ledger-panel">
      <div class="section-title">
        <div>
          <h3>库存列表</h3>
          <span>共 {{ total }} 条记录</span>
        </div>
        <b>{{ selectedIds.length }} 项已选</b>
      </div>

      <div class="toolbar">
        <input
          v-model.trim="keywordInput"
          type="text"
          placeholder="按物品名称查询"
          @keyup.enter="applySearch"
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
        <small>排序</small>
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
        @update:page="page = $event"
      />
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

    <div v-if="drawerOpen" class="drawer-backdrop" @click.self="closeDrawer">
      <aside class="inventory-drawer">
        <div class="drawer-head">
          <div>
            <small>{{ editingItem ? "EDIT ITEM" : "CREATE ITEM" }}</small>
            <h3>{{ editingItem ? editingItem.item_name : "新增入库" }}</h3>
          </div>
          <button type="button" class="icon-button" @click="closeDrawer">
            ×
          </button>
        </div>

        <div class="form-grid">
          <label>
            <span>物品名称</span>
            <input v-model="editForm.item_name" type="text" />
          </label>
          <label>
            <span>物品类型</span>
            <input v-model="editForm.item_type" type="text" />
          </label>
          <label>
            <span>数量</span>
            <input
              v-model.number="editForm.item_number"
              type="number"
              min="1"
            />
          </label>
          <label>
            <span>单价</span>
            <input
              v-model.number="editForm.item_price"
              type="number"
              min="0.01"
              step="0.01"
            />
          </label>
          <label>
            <span>总价</span>
            <input type="text" :value="editSummaryTotal" readonly />
          </label>
          <label>
            <span>库存记录</span>
            <input
              type="text"
              :value="editingItem ? '系统已建档' : '新增后生成'"
              readonly
            />
          </label>
          <label>
            <span>生产日期</span>
            <input v-model="editForm.item_productiondate" type="date" />
          </label>
          <label>
            <span>到期日期</span>
            <input v-model="editForm.item_expirationdate" type="date" />
          </label>
        </div>

        <div class="meta-strip">
          <span>{{
            editingItem ? "当前库存 " + editingItem.item_number : "新增库存记录"
          }}</span>
          <span>{{
            editingItem ? "修改后 " + editForm.item_number : "待入库"
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
          <button class="ghost" @click="closeDrawer">取消</button>
          <button @click="saveEdit">
            {{ editingItem ? "保存修改" : "新增入库" }}
          </button>
        </div>

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
            <p class="delete-title">新增模式不可删除</p>
            <small>保存为库存记录后，才能执行删除操作。</small>
          </template>
        </section>
      </aside>
    </div>
  </section>
</template>

<script lang="ts">
import {
  computed,
  defineComponent,
  onMounted,
  reactive,
  ref,
  watch,
} from "vue";
import WarehouseStatCard from "@/modules/warehouse-admin/components/WarehouseStatCard.vue";
import {
  WarehouseCreatePayload,
  WarehouseItem,
} from "@/modules/warehouse-admin/api/types";
import { warehouseAdminApi } from "@/modules/warehouse-admin/api/warehouseAdminApi";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";

export default defineComponent({
  name: "WarehouseAdminInventory",
  components: { WarehouseStatCard, AppPager },
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

    const items = ref<WarehouseItem[]>([]);
    const total = ref(0);
    const page = ref(1);
    const pageSize = 10;
    const keywordInput = ref("");
    const keyword = ref("");
    const activeType = ref("全部");
    const sortKey = ref<"name" | "stock" | "price" | "total" | "expiry">(
      "name"
    );
    const editingItem = ref<WarehouseItem | null>(null);
    const deletingItem = ref<WarehouseItem | null>(null);
    const drawerOpen = ref(false);
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
     * 进入仓库仪表盘时通过列表接口获取库存数据。
     */
    const loadItems = async () => {
      const result = await warehouseAdminApi.list({
        keyword: keyword.value.trim(),
        itemType: activeType.value,
        sortKey: sortKey.value,
        page: page.value,
        pageSize,
      });
      items.value = result.items;
      total.value = result.total;
    };

    const filteredItems = computed(() => items.value);
    const placeholderRows = computed(() =>
      Array.from(
        { length: Math.max(0, pageSize - filteredItems.value.length) },
        (_, index) => index + 1
      )
    );
    const totalPages = computed(() =>
      Math.max(1, Math.ceil(total.value / pageSize))
    );

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
      drawerOpen.value = true;
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
      drawerOpen.value = true;
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

    const closeDrawer = () => {
      drawerOpen.value = false;
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
        await loadItems();
        closeDrawer();
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
        await loadItems();
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
        await loadItems();
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
        await loadItems();
        closeDrawer();
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

    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
      page.value = 1;
      void loadItems();
    };

    onMounted(() => {
      // 页面首次进入时优先复用仓库缓存，只有过期或脏数据时才会重拉。
      void loadItems();
      void store.dispatch("warehouseAdmin/ensureLogs");
    });

    watch([activeType, sortKey], () => {
      page.value = 1;
      void loadItems();
    });

    watch(page, () => {
      void loadItems();
    });

    return {
      items,
      total,
      page,
      totalPages,
      placeholderRows,
      keywordInput,
      keyword,
      activeType,
      sortKey,
      typeFilters,
      sortOptions,
      filteredItems,
      allVisibleSelected,
      editingItem,
      deletingItem,
      drawerOpen,
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
      closeDrawer,
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
      applySearch,
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
  color: #1e342b;
}

.command-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  border: 1px solid #dfe7df;
  border-radius: 12px;
  padding: 16px 18px;
  background: #ffffff;
  box-shadow: 0 10px 24px rgba(32, 62, 45, 0.06);
}

.command-head p,
.command-head h2,
.command-head span {
  margin: 0;
}

.command-head p {
  color: #6d7f72;
  font-size: 11px;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.command-head h2 {
  margin: 4px 0;
  font-size: 26px;
  line-height: 1.15;
  color: #1c3329;
}

.command-head span {
  color: #6a7a70;
  font-size: 13px;
}

.command-head button {
  flex: 0 0 auto;
  border: 0;
  border-radius: 8px;
  padding: 11px 16px;
  background: #245849;
  color: #ffffff;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 10px 20px rgba(36, 88, 73, 0.18);
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

.ledger-panel,
.warning-panel,
.delete-panel {
  border-radius: 12px;
  border: 1px solid #dfe7df;
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(35, 62, 46, 0.06);
}

.ledger-panel {
  min-width: 0;
  padding: 16px;
}

.warning-panel,
.delete-panel {
  padding: 14px;
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
  color: #1d3429;
  font-size: 16px;
}

.section-title b {
  flex: 0 0 auto;
  border-radius: 8px;
  padding: 6px 10px;
  background: #eef5ef;
  color: #3d735d;
  font-size: 12px;
}

.delete-title {
  margin: 0;
  font-size: 16px;
  color: #163d42;
}

.section-title span,
.delete-panel small {
  color: #6d7b72;
  font-size: 12px;
}

.toolbar {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 10px;
  margin-bottom: 12px;
}

.toolbar input,
.inventory-drawer input,
.delete-panel input {
  min-width: 0;
  width: 100%;
  box-sizing: border-box;
  border: 1px solid #d8e2da;
  border-radius: 8px;
  padding: 11px 12px;
  background: #ffffff;
  color: #1d3429;
  outline: none;
}

.toolbar input::placeholder,
.inventory-drawer input::placeholder,
.delete-panel input::placeholder {
  color: #93a097;
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
  border: 1px solid #d8e2da;
  background: #ffffff;
  color: #5e7066;
  font-size: 12px;
  cursor: pointer;
}

.chips button {
  padding: 10px 12px;
}

.chips button.active,
.sort-strip span.active {
  color: #ffffff;
  border-color: #245849;
  background: #245849;
}

.sort-strip {
  margin-bottom: 12px;
  padding: 10px;
  border-radius: 10px;
  background: #f4f7f4;
}

.sort-strip small {
  color: #6d7b72;
  font-size: 12px;
}

.sort-strip span {
  padding: 9px 14px;
}

.grid-table {
  border: 1px solid #dfe7df;
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
  color: #6b786e;
  font-size: 12px;
  border-bottom: 1px solid #dfe7df;
  background: #f4f7f4;
}

.grid-row {
  color: #20382d;
  font-size: 13px;
  border-top: 1px solid #edf2ee;
  transition: background 0.18s ease;
}

.grid-row:hover {
  background: #f8fbf8;
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
  accent-color: #245849;
  cursor: pointer;
}

.name-cell b {
  display: block;
}

.name-cell small {
  color: #7a877e;
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
  border: 1px solid #d8e2da;
  border-radius: 8px;
  padding: 10px 12px;
  background: #ffffff;
  color: #245849;
  font-weight: 700;
  cursor: pointer;
}

.panel-actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.panel-actions .ghost {
  background: #f4f7f4;
}

.panel-actions button:last-child {
  border-color: #245849;
  background: #245849;
  color: #ffffff;
  font-weight: 700;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.form-grid label {
  display: grid;
  gap: 6px;
  min-width: 0;
}

.form-grid label span {
  color: #617268;
  font-size: 12px;
}

.meta-strip {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
  margin: 14px 0;
}

.meta-strip span {
  padding: 8px 12px;
  border-radius: 8px;
  background: #f4f7f4;
  color: #617268;
  font-size: 12px;
}

.movement-box {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto auto;
  gap: 8px;
  margin: 10px 0 14px;
}

.movement-box button {
  border: 1px solid #d8e2da;
  border-radius: 8px;
  padding: 0 14px;
  background: #f8faf8;
  color: #245849;
  font-weight: 800;
  cursor: pointer;
}

.movement-box button:disabled {
  color: #9dafb1;
  cursor: not-allowed;
  opacity: 0.68;
}

.warning-panel {
  background: #ffffff;
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
  border: 1px solid #ead8b5;
  border-radius: 10px;
  padding: 10px 12px;
  background: #fffaf1;
  color: #20382d;
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
  border: 1px dashed #cfdad2;
  border-radius: 10px;
  padding: 18px;
  color: #7a877e;
  text-align: center;
}

.delete-panel {
  margin-top: 16px;
  background: #fffafa;
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

.drawer-backdrop {
  position: fixed;
  inset: 0;
  z-index: 50;
  display: flex;
  justify-content: flex-end;
  background: rgba(24, 35, 30, 0.28);
}

.inventory-drawer {
  width: min(520px, 100%);
  height: 100%;
  box-sizing: border-box;
  overflow-y: auto;
  padding: 20px;
  background: #ffffff;
  box-shadow: -18px 0 44px rgba(24, 42, 32, 0.18);
}

.drawer-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 18px;
}

.drawer-head small {
  color: #6d7b72;
  font-size: 11px;
  letter-spacing: 0.12em;
}

.drawer-head h3 {
  margin: 4px 0 0;
  color: #1d3429;
  font-size: 20px;
}

.icon-button {
  display: grid;
  place-items: center;
  width: 34px;
  height: 34px;
  border: 1px solid #d8e2da;
  border-radius: 8px;
  background: #ffffff;
  color: #20382d;
  font-size: 22px;
  line-height: 1;
  cursor: pointer;
}

@media (max-width: 1280px) {
  .warning-list {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 1080px) {
  .stats-row {
    grid-template-columns: repeat(2, minmax(0, 1fr));
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

  .ledger-panel,
  .delete-panel {
    border-radius: 12px;
  }

  .ledger-panel,
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
  .movement-box,
  .warning-list {
    grid-template-columns: 1fr;
  }

  .inventory-drawer {
    padding: 16px;
  }
}
</style>
