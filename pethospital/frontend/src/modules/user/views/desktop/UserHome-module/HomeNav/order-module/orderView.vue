<template>
  <section class="order-page">
    <div class="ord-head">
      <h2>我的订单</h2>
      <div class="seg">
        <button
          class="seg__btn"
          :class="{ 'seg__btn--active': activeTab === 'order' }"
          @click="switchTab('order')"
        >
          诊单
        </button>
        <button
          class="seg__btn"
          :class="{ 'seg__btn--active': activeTab === 'reservation' }"
          @click="switchTab('reservation')"
        >
          预约记录
        </button>
      </div>
    </div>

    <section class="order-toolbar" ref="homeOrderTopRef">
      <div
        class="toolbar-search"
        :class="{ 'toolbar-search--open': openSearch }"
      >
        <span>{{ currentSearchType }}</span>
        <input
          v-model.trim="searchQuery"
          type="text"
          :placeholder="searchPlaceholder"
          @keyup.enter="confirmSearch"
          @focus="openSearch = true"
          @blur="handleInputBlur"
        />
        <button
          type="button"
          class="toolbar-search__button"
          :disabled="searchLoading"
          @mousedown.prevent
          @click="confirmSearch"
        >
          {{ searchLoading ? "搜索中" : "搜索" }}
        </button>
      </div>

      <div v-if="openSearch" class="search-popover">
        <div class="search-popover__header">
          <strong>搜索记录</strong>
          <button
            v-if="historyOrders.length > 0"
            class="search-popover__clear"
            @click="clearSearchHistory"
          >
            清除历史
          </button>
        </div>
        <div v-if="historyOrders.length > 0" class="search-popover__history">
          <button
            v-for="order in historyOrders"
            :key="order.id"
            class="search-popover__chip"
            @click="buttonClick(getItemDisplayName(order))"
          >
            {{ getItemDisplayName(order) }}
          </button>
        </div>
        <p v-else class="search-popover__empty">还没有搜索记录。</p>
      </div>

      <div class="toolbar-actions">
        <button class="toolbar-pill" @click="changeSort('time')">
          时间 {{ sortKey === "time" ? sortLabel : "" }}
        </button>
        <button class="toolbar-pill" @click="changeSort('price')">
          价格 {{ sortKey === "price" ? sortLabel : "" }}
        </button>
        <button class="toolbar-edit" @click="toggleEditMode">
          {{ editTab ? "完成编辑" : "编辑模式" }}
        </button>
      </div>
    </section>

    <section
      class="order-list-panel"
      :class="{ 'order-list-panel--editing': editTab }"
    >
      <div class="order-list-panel__head">
        <div>
          <p>
            {{ activeTab === "order" ? "Order Feed" : "Reservation Feed" }}
          </p>
          <h3>{{ activeTab === "order" ? "订单列表" : "预约记录列表" }}</h3>
        </div>
        <span>{{
          searchQuery ? `关键词：${searchQuery}` : "未设置筛选关键词"
        }}</span>
      </div>

      <AsyncViewState
        v-if="pageLoading || pageErrorMessage"
        :loading="pageLoading"
        :error="pageErrorMessage"
        loading-text="正在同步订单与预约记录"
        @retry="loadOrderPageData"
      />

      <div v-else-if="visibleItems.length > 0" class="order-list">
        <article
          v-for="item in visibleItems"
          :key="item.id"
          class="order-item"
          :class="{ 'order-item--editing': editTab }"
        >
          <label v-if="editTab" class="order-item__checkbox">
            <input
              :checked="isSelected(item.id)"
              type="checkbox"
              @change="ordersButton(item.id)"
            />
          </label>

          <button class="order-item__content" @click="goToDetail(item)">
            <div class="order-item__main">
              <strong>{{ getItemDisplayName(item) }}</strong>
              <span>ID {{ item.id }}</span>
            </div>
            <div class="order-item__meta">
              <div class="order-item_meta-Introduction">
                <small>简介</small>
                <span>{{ getItemDescription(item) }}</span>
              </div>
              <div>
                <small>时间</small>
                <span>{{ formatTimeValue(item) }}</span>
              </div>
              <div>
                <small>价格</small>
                <span>{{ formatPrice(getItemPrice(item)) }}</span>
              </div>
              <div class="order-item__tag">
                {{ activeTab === "order" ? "查看订单详情" : "查看预约详情" }}
              </div>
            </div>
          </button>
        </article>
      </div>

      <div v-else class="order-empty">
        <strong>当前没有可展示的记录</strong>
        <span>可以尝试切换标签、清空关键词，或等待新的预约与订单同步。</span>
      </div>
    </section>

    <section v-if="editTab" class="batch-actions">
      <button class="batch-actions__ghost" @click="moveSelectedToFavorites">
        移入收藏
      </button>
      <button class="batch-actions__danger" @click="deleteSelected">
        删除所选
      </button>
    </section>
  </section>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch } from "vue";
import { useStore } from "vuex";
import { getHttpErrorMessage } from "@/api/httpError";
import { storeKey } from "@/app/store";
import AsyncViewState from "@/shared/components/AsyncViewState.vue";
import { orderApi, reservationApi } from "@/modules/user/api/userApi";
import { useRoute, useRouter } from "vue-router";
import {
  clearOrderSearchHistory as clearStoredOrderSearchHistory,
  readOrderSearchHistory,
  saveOrderSearchKeyword,
} from "@/modules/user/utils/userPreferenceStorage";

type SearchableOrderItem = {
  id: number;
  user_id?: number;
  user_name?: string;
  phone?: string;
  doctor_id?: number;
  doctor_name?: string;
  pet_id?: number;
  pet_name?: string;
  reservation_type?: string;
  date?: string;
  time_slot?: string;
  schedule?: string;
  status?: string;
  price?: number;
  created_at?: string;
  order_type?: string;
  order_data?: string;
  order_status?: string;
  order_totalprice?: number;
};

type SortKey = "time" | "price";
type SortDirection = "asc" | "desc";

const store = useStore(storeKey);
const router = useRouter();
const route = useRoute();

const activeTab = ref<"order" | "reservation">("order");
const editTab = ref(false);
const searchQuery = ref("");
const searchLoading = ref(false);
const searchResults = ref<SearchableOrderItem[] | null>(null);
const openSearch = ref(false);
const homeOrderTopRef = ref<HTMLDivElement | null>(null);
const choiceActive = ref<Record<number, boolean>>({});
const historyOrders = ref<SearchableOrderItem[]>([]);
const favoriteOrders = ref<SearchableOrderItem[]>([]);
const sortKey = ref<SortKey>("time");
const sortDirection = ref<SortDirection>("desc");
const pageErrorMessage = ref("");

const MAX_HISTORY_COUNT = 15;

const basePath = computed(() => "/user");

/**
 * 订单页与预约记录页都直接复用用户端缓存，避免来回切页时重复拉取。
 */
const orders = computed<SearchableOrderItem[]>(
  () => store.state.userPortal.orderSummaries
);
const reservationOrder = computed<SearchableOrderItem[]>(
  () => store.state.userPortal.reservationRecords
);

const currentSource = computed(
  () =>
    searchResults.value ??
    (activeTab.value === "order" ? orders.value : reservationOrder.value)
);

const pageLoading = computed(() =>
  Boolean(
    store.state.userPortal.orderSummariesMeta.loading ||
      store.state.userPortal.reservationRecordsMeta.loading
  )
);

const visibleItems = computed(() =>
  [...currentSource.value].sort((a, b) => {
    const left =
      sortKey.value === "price"
        ? getItemPrice(a) || 0
        : getItemSortTimeValue(a);
    const right =
      sortKey.value === "price"
        ? getItemPrice(b) || 0
        : getItemSortTimeValue(b);
    return sortDirection.value === "asc" ? left - right : right - left;
  })
);

const sortLabel = computed(() => (sortDirection.value === "asc" ? "↑" : "↓"));
const currentSearchType = computed(() =>
  activeTab.value === "order" ? "orders" : "reservations"
);
const searchPlaceholder = computed(() =>
  activeTab.value === "order"
    ? "按诊单编号、宠物名、医生名或订单状态搜索"
    : "按预约编号、宠物名、医生名、日期或预约状态搜索"
);

const switchTab = (tab: "order" | "reservation") => {
  activeTab.value = tab;
  searchQuery.value = "";
  searchResults.value = null;
  openSearch.value = false;
  editTab.value = false;
  choiceActive.value = {};
};

const confirmSearch = async () => {
  const keyword = searchQuery.value.trim();
  if (!keyword) {
    searchResults.value = null;
    openSearch.value = false;
    return;
  }

  searchLoading.value = true;
  pageErrorMessage.value = "";
  try {
    searchResults.value =
      activeTab.value === "order"
        ? await orderApi.searchOrderSummaries(keyword)
        : await reservationApi.searchReservationSummaries(keyword);
    await orderApi.updateSearchHistory(keyword);
    historyOrders.value = saveOrderSearchKeyword(
      historyOrders.value,
      keyword,
      (nextKeyword) => ({
        id: Date.now(),
        pet_name: nextKeyword,
      }),
      getItemDisplayName,
      MAX_HISTORY_COUNT,
      "user",
      currentSearchType.value
    );
    openSearch.value = false;
  } catch (error) {
    pageErrorMessage.value = getHttpErrorMessage(error, "搜索失败，请稍后重试");
  } finally {
    searchLoading.value = false;
  }
};

const clearSearchHistory = () => {
  historyOrders.value = [];
  clearStoredOrderSearchHistory("user", currentSearchType.value);
};

const loadSearchHistory = () => {
  historyOrders.value = readOrderSearchHistory<SearchableOrderItem>(
    "user",
    currentSearchType.value
  );
};

const handleClickOutside = (event: MouseEvent) => {
  if (
    homeOrderTopRef.value &&
    !homeOrderTopRef.value.contains(event.target as Node)
  ) {
    openSearch.value = false;
  }
};

const handleInputBlur = () => {
  setTimeout(() => {
    openSearch.value = false;
  }, 200);
};

const setChoiceActive = (id: number) => {
  choiceActive.value[id] = !choiceActive.value[id];
};

const ordersButton = (id: number) => {
  setChoiceActive(id);
};

const isSelected = (id: number) => Boolean(choiceActive.value[id]);

const buttonClick = (name: string) => {
  searchQuery.value = name;
  void confirmSearch();
};

const changeSort = (type: SortKey) => {
  if (sortKey.value === type) {
    sortDirection.value = sortDirection.value === "asc" ? "desc" : "asc";
    return;
  }
  sortKey.value = type;
  sortDirection.value = "desc";
};

const toggleEditMode = () => {
  editTab.value = !editTab.value;
  if (!editTab.value) {
    choiceActive.value = {};
  }
};

const moveSelectedToFavorites = () => {
  const selected = currentSource.value.filter((item) => isSelected(item.id));
  const merged = [...favoriteOrders.value];

  selected.forEach((item) => {
    if (!merged.some((favorite) => favorite.id === item.id)) {
      merged.push(item);
    }
  });

  favoriteOrders.value = merged;
  choiceActive.value = {};
  editTab.value = false;
};

const deleteSelected = async () => {
  const selectedIds = Object.entries(choiceActive.value)
    .filter(([, checked]) => checked)
    .map(([id]) => Number(id));

  if (selectedIds.length === 0) {
    return;
  }

  if (activeTab.value === "order") {
    await store.dispatch(
      "userPortal/saveOrderSummaries",
      orders.value.filter((item) => !selectedIds.includes(item.id))
    );
  } else {
    await Promise.all(
      selectedIds.map((id) =>
        store.dispatch("userPortal/deleteReservationRecord", id)
      )
    );
  }

  choiceActive.value = {};
  editTab.value = false;
};

const loadOrderPageData = async () => {
  pageErrorMessage.value = "";
  try {
    await store.dispatch("userPortal/ensureOrderPageData", { force: true });
    searchResults.value = null;
  } catch (error) {
    pageErrorMessage.value = getHttpErrorMessage(
      error,
      "订单与预约记录加载失败，请稍后重试"
    );
  }
};

watch(searchQuery, () => {
  if (!searchQuery.value.trim()) {
    searchResults.value = null;
  }
});

watch(activeTab, () => {
  loadSearchHistory();
});

const goToDetail = (item: SearchableOrderItem) => {
  if (editTab.value) {
    ordersButton(item.id);
  } else {
    router.push({
      path: `${basePath.value}/orderDetail`,
      query: {
        tab: activeTab.value,
        id: String(item.id),
      },
    });
  }
};

const formatPrice = (value?: number) =>
  typeof value === "number" ? `¥ ${value.toFixed(2)}` : "待结算";

const getItemPrice = (item: SearchableOrderItem) =>
  typeof item.order_totalprice === "number"
    ? item.order_totalprice
    : item.price;

const getItemDisplayName = (item: SearchableOrderItem) =>
  item.pet_name || item.user_name || "预约记录";

const getItemDescription = (item: SearchableOrderItem) => {
  if (activeTab.value === "reservation") {
    return item.reservation_type || "预约记录";
  }

  return [item.order_type || "普通诊单", item.doctor_name]
    .filter(Boolean)
    .join(" · ");
};

const getItemSortTimeValue = (item: SearchableOrderItem) => {
  if (activeTab.value === "reservation") {
    const reservationTime =
      item.schedule || [item.date, item.time_slot].filter(Boolean).join(" ");
    const timestamp = new Date(
      reservationTime || item.created_at || ""
    ).getTime();
    return Number.isNaN(timestamp) ? 0 : timestamp;
  }

  const timestamp = new Date(item.order_data || "").getTime();
  return Number.isNaN(timestamp) ? 0 : timestamp;
};

const formatTimeValue = (item: SearchableOrderItem) => {
  if (activeTab.value === "reservation") {
    const reservationTime =
      item.schedule || [item.date, item.time_slot].filter(Boolean).join(" ");

    if (reservationTime) {
      return reservationTime;
    }

    if (item.created_at) {
      return item.created_at;
    }
  }

  if (item.order_data) {
    return item.order_data;
  }

  return "待同步";
};

onMounted(() => {
  if (route.query.tab === "reservation") {
    activeTab.value = "reservation";
  }

  document.addEventListener("click", handleClickOutside);
  loadSearchHistory();
  /**
   * 订单页进入时通过 RESTful 获取一次订单和预约摘要，不复用本地持久化缓存。
   */
  void loadOrderPageData();
});

onBeforeUnmount(() => {
  document.removeEventListener("click", handleClickOutside);
});
</script>

<style scoped lang="scss">
.order-page {
  display: grid;
  gap: 14px;
  position: relative;
}

.ord-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

.ord-head h2 {
  margin: 0;
  font-size: 20px;
  font-weight: 800;
  letter-spacing: -0.01em;
  color: #1f3a36;
}

.seg {
  display: inline-flex;
  padding: 4px;
  gap: 2px;
  background: #ffffff;
  border: 1px solid #efe7dc;
  border-radius: 12px;
}

.seg__btn {
  height: 36px;
  padding: 0 18px;
  border: 0;
  border-radius: 9px;
  background: transparent;
  color: #6b7d77;
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
}

.seg__btn--active {
  background: #e7f5f1;
  color: #1f7a6c;
}

.order-toolbar,
.order-list-panel,
.batch-actions {
  border-radius: 16px;
  border: 1px solid #efe7dc;
  background: #ffffff;
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.06);
}

.order-list-panel__head p {
  margin: 0;
  color: #2f9e8f;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 10px;
  font-weight: 700;
}

.order-list-panel__head h3 {
  margin: 0;
  color: #1f3a36;
  font-size: 16px;
}

.order-list-panel__head span,
.order-empty span {
  color: #6b7d77;
  line-height: 1.6;
  font-size: 13px;
}

.order-toolbar {
  position: relative;
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 16px;
  align-items: center;
  padding: 12px 16px;
}

.toolbar-search {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  align-items: center;
  height: 56px;
  max-height: 56px;
  min-height: 56px;
  border-radius: 20px;
  border: 1px solid rgba(47, 158, 143, 0.1);
  background: rgba(255, 255, 255, 0.64);
  overflow: hidden;
}

.toolbar-search span {
  min-width: 118px;
  padding: 0 14px;
  text-align: center;
  font-size: 12px;
  font-weight: 800;
  color: #2f9e8f;
}

.toolbar-search input {
  min-width: 0;
  width: calc(80% - 6px);
  min-height: 46px;
  padding: 0 18px 0 8px;
  border: none;
  background: transparent;
  color: #1f3a36;
  font-size: 16px;
  line-height: 46px;
  outline: none;
}

.toolbar-search__button {
  height: 42px;
  margin-right: 8px;
  padding: 0 16px;
  border: none;
  border-radius: 14px;
  background: linear-gradient(135deg, #2f9e8f, #1f7a6c);
  color: #fffdfa;
  font-weight: 800;
  cursor: pointer;
}

.toolbar-search__button:disabled {
  opacity: 0.58;
  cursor: not-allowed;
}

.search-popover {
  position: absolute;
  top: calc(100% - 6px);
  left: 22px;
  width: min(520px, calc(100% - 44px));
  padding: 16px;
  border-radius: 16px;
  border: 1px solid rgba(47, 158, 143, 0.1);
  background: rgba(255, 253, 250, 0.96);
  box-shadow: 0 18px 40px rgba(47, 158, 143, 0.1);
  z-index: 5;
}

.search-popover__header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.search-popover__header strong {
  font-size: 14px;
  color: #1f3a36;
}

.search-popover__clear {
  border: none;
  background: none;
  color: #c2671b;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
}

.search-popover__history {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.search-popover__chip {
  padding: 10px 14px;
  border: none;
  border-radius: 999px;
  background: rgba(47, 158, 143, 0.08);
  color: #1f3a36;
  cursor: pointer;
}

.search-popover__empty {
  margin: 0;
  color: #6b7d77;
  font-size: 13px;
}

.toolbar-actions {
  display: flex;
  flex-wrap: nowrap;
  align-items: center;
  justify-content: flex-end;
  gap: 10px;
}

.toolbar-pill,
.toolbar-edit,
.batch-actions button,
.order-item__content {
  border: none;
  cursor: pointer;
}

.toolbar-pill,
.toolbar-edit {
  padding: 0 16px;
  border-radius: 999px;
  font-weight: 700;
}

.toolbar-pill {
  min-height: 56px;
  height: 56px;
  background: rgba(47, 158, 143, 0.08);
  color: #1f3a36;
  white-space: nowrap;
}

.toolbar-edit {
  min-height: 56px;
  height: 56px;
  background: linear-gradient(135deg, #2f9e8f, #1f7a6c);
  color: #fffdfa;
  white-space: nowrap;
}

.order-summary {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
}

.order-summary article {
  min-height: 125px;
  padding: 16px 20px;
}

.order-summary p {
  margin: 0 0 8px;
  color: #2f9e8f;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.order-summary strong {
  display: block;
  margin-bottom: 6px;
  font-size: 26px;
  color: #1f3a36;
}

.order-summary span {
  color: #6b7d77;
  font-size: 13px;
  line-height: 1.65;
}

.order-list-panel {
  display: grid;
  gap: 12px;
  align-content: start;
  padding: 18px;
}

.order-list-panel__head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
}

.order-list {
  display: grid;
  align-content: start;
  gap: 12px;
}

.order-item {
  display: grid;
  grid-template-columns: minmax(0, 1fr);
  gap: 12px;
  align-items: center;
  padding: 14px 16px;
  border-radius: 14px;
  border: 1px solid #efe7dc;
  background: #fffdfa;
}

.order-item--editing {
  grid-template-columns: auto minmax(0, 1fr);
}

.order-item__checkbox {
  display: grid;
  place-items: center;
}

.order-item__checkbox input {
  width: 18px;
  height: 18px;
  accent-color: #2f9e8f;
  cursor: pointer;
}

.order-item__content {
  width: 100%;
  display: grid;
  grid-template-columns: minmax(0, auto) minmax(360px, auto);
  gap: 14px;
  align-items: center;
  padding: 0;
  background: transparent;
  text-align: left;
  color: inherit;
}

.order-item__main strong {
  display: block;
  margin-bottom: 6px;
  font-size: 20px;
  color: #1f3a36;
}

.order-item__main span,
.order-item__meta small {
  color: #6b7d77;
  font-size: 12px;
}

.order-item__meta {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 18px;
  width: 100%;

  small {
    font-size: 18px;
    text-align: center;
  }
  span {
    font-size: 14px;
    color: #1f3a36;
    font-weight: 700;
    text-align: center;
  }
  .order-item_meta-Introduction {
    min-width: 450px;
  }
}

.order-item__meta div {
  display: grid;
  gap: 4px;
  min-width: 84px;
}

.order-item__meta span {
  color: #1f3a36;
  font-weight: 700;
}

.order-item__tag {
  padding: 10px 14px;
  border-radius: 999px;
  background: rgba(47, 158, 143, 0.08);
  color: #1f7a6c;
  font-size: 12px;
  font-weight: 700;
}

.order-empty {
  display: grid;
  justify-items: center;
  gap: 8px;
  padding: 34px 18px;
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.58);
  text-align: center;
}

.order-empty strong {
  font-size: 20px;
  color: #1f3a36;
}

.batch-actions {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  padding: 16px 20px;
  flex: 0 0 auto;
}

.batch-actions__ghost,
.batch-actions__danger {
  min-width: 140px;
  padding: 12px 18px;
  border-radius: 16px;
  font-weight: 700;
}

.batch-actions__ghost {
  background: rgba(47, 158, 143, 0.08);
  color: #1f7a6c;
}

.batch-actions__danger {
  background: linear-gradient(135deg, #c2671b, #c2671b);
  color: #fffdfa;
}

@media (max-width: 1100px) {
  .order-page,
  .order-summary,
  .order-item__content {
    grid-template-columns: 1fr;
  }

  .order-sidebar {
    position: static;
  }

  .order-stage__toolbar {
    grid-template-columns: minmax(0, 1fr) auto;
    justify-content: center;
    gap: 12px;
    padding: 0 18px;
  }

  .toolbar-search {
    width: min(100%, 720px);
    justify-self: center;
  }

  .toolbar-search input {
    width: 100%;
  }

  .toolbar-actions {
    justify-content: flex-start;
    gap: 8px;
  }

  .toolbar-pill,
  .toolbar-edit {
    padding: 0 14px;
  }

  .batch-actions {
    justify-content: flex-start;
  }
}
</style>
