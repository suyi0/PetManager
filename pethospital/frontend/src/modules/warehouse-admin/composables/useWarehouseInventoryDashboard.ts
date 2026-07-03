// Encapsulates warehouse inventory dashboard state, derived values, and actions.
import { computed, onMounted, reactive, ref, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import {
  WarehouseCreatePayload,
  WarehouseItem,
} from "@/modules/warehouse-admin/api/types";
import { warehouseAdminApi } from "@/modules/warehouse-admin/api/warehouseAdminApi";
import {
  calculateTotalPages,
  createPlaceholderIndexes,
} from "@/shared/utils/pagination";

export type WarehouseInventorySortKey =
  | "name"
  | "stock"
  | "price"
  | "total"
  | "expiry";

export const typeFilters = ["全部", "药品", "耗材", "营养品"];

export const sortOptions = [
  { key: "name", label: "名称" },
  { key: "stock", label: "库存" },
  { key: "price", label: "单价" },
  { key: "total", label: "总价" },
  { key: "expiry", label: "到期日期" },
] as const;

export const useWarehouseInventoryDashboard = () => {
  const store = useStore(storeKey);

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
  const sortKey = ref<WarehouseInventorySortKey>("name");
  const editingItem = ref<WarehouseItem | null>(null);
  const deletingItem = ref<WarehouseItem | null>(null);
  const drawerOpen = ref(false);
  const selectedIds = ref<number[]>([]);
  const deleteConfirmText = ref("");
  const movementQuantity = ref(1);
  const statusMessage = ref("");
  const statusType = ref<"info" | "error">("info");
  const editForm = reactive<WarehouseCreatePayload>(createEmptyForm());

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
    createPlaceholderIndexes(pageSize, filteredItems.value.length)
  );
  const totalPages = computed(() => calculateTotalPages(total.value, pageSize));

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
        (item) => item.days_until_expire !== null && item.days_until_expire <= 7
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

  const updateEditFormField = <Key extends keyof WarehouseCreatePayload>(
    field: Key,
    value: WarehouseCreatePayload[Key]
  ) => {
    editForm[field] = value;
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

  const confirmDelete = async () => {
    if (!deletingItem.value) return;

    try {
      await store.dispatch("warehouseAdmin/deleteItem", deletingItem.value.id);
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
    updateEditFormField,
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
};
