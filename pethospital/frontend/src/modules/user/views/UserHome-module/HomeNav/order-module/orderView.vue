<template>
  <section class="order-page">
    <aside class="order-sidebar">
      <div class="order-sidebar__hero">
        <p>Orders</p>
        <h2>订单中心</h2>
        <span>统一查看普通订单与预约记录，支持搜索、排序和批量整理。</span>
      </div>

      <button
        class="order-sidebar__item"
        :class="{ 'order-sidebar__item--active': activeTab === 'order' }"
        @click="switchTab('order')"
      >
        订单
      </button>
      <button
        class="order-sidebar__item"
        :class="{ 'order-sidebar__item--active': activeTab === 'reservation' }"
        @click="switchTab('reservation')"
      >
        预约记录
      </button>
    </aside>

    <div class="order-stage">
      <section class="order-stage__toolbar" ref="homeOrderTopRef">
        <div
          class="toolbar-search"
          :class="{ 'toolbar-search--open': openSearch }"
        >
          <span>搜索</span>
          <input
            v-model.trim="searchQuery"
            type="text"
            placeholder="按名称搜索订单或预约"
            @keyup.enter="confirmSearch"
            @focus="openSearch = true"
            @blur="handleInputBlur"
          />
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
              @click="buttonClick(order.name)"
            >
              {{ order.name }}
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

      <section class="order-summary">
        <article>
          <p>当前列表</p>
          <strong>{{ activeTab === "order" ? "普通订单" : "预约记录" }}</strong>
          <span>切换左侧标签可查看另一类记录。</span>
        </article>
        <article>
          <p>结果数量</p>
          <strong>{{ visibleItems.length }}</strong>
          <span>搜索和排序会即时作用于当前页结果。</span>
        </article>
        <article>
          <p>已选条数</p>
          <strong>{{ selectedCount }}</strong>
          <span>编辑模式下支持批量收藏与批量删除。</span>
        </article>
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

        <div v-if="visibleItems.length > 0" class="order-list">
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
                <strong>{{ item.name }}</strong>
                <span>ID {{ item.id }}</span>
              </div>
              <div class="order-item__meta">
                <div class="order-item_meta-Introduction">
                  <small>简介</small>
                  <span>{{}}</span>
                </div>
                <div>
                  <small>时间</small>
                  <span>{{ formatTimeValue(item.time) }}</span>
                </div>
                <div>
                  <small>价格</small>
                  <span>{{ formatPrice(item.price) }}</span>
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
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";
import { useRoute, useRouter } from "vue-router";
import { orderApi, reservationApi } from "@/modules/user/api/userApi";

type SearchableOrderItem = {
  id: number;
  name: string;
  time?: number;
  price?: number;
};

type SortKey = "time" | "price";
type SortDirection = "asc" | "desc";

const store = useStore(storeKey);
const router = useRouter();
const route = useRoute();

const activeTab = ref<"order" | "reservation">("order");
const editTab = ref(false);
const searchQuery = ref("");
const openSearch = ref(false);
const orders = ref<SearchableOrderItem[]>([]);
const reservationOrder = ref<SearchableOrderItem[]>([]);
const homeOrderTopRef = ref<HTMLDivElement | null>(null);
const choiceActive = ref<Record<number, boolean>>({});
const historyOrders = ref<SearchableOrderItem[]>([]);
const sortKey = ref<SortKey>("time");
const sortDirection = ref<SortDirection>("desc");

const MAX_HISTORY_COUNT = 15;

const basePath = computed(() =>
  route.path.startsWith("/preview/user") ? "/preview/user" : "/user"
);

const currentSource = computed(() =>
  activeTab.value === "order" ? orders.value : reservationOrder.value
);

const visibleItems = computed(() => {
  const keyword = searchQuery.value.trim().toLowerCase();
  const rows = currentSource.value.filter((item) =>
    keyword ? item.name.toLowerCase().includes(keyword) : true
  );

  return [...rows].sort((a, b) => {
    const left = sortKey.value === "price" ? a.price || 0 : a.time || 0;
    const right = sortKey.value === "price" ? b.price || 0 : b.time || 0;
    return sortDirection.value === "asc" ? left - right : right - left;
  });
});

const selectedCount = computed(
  () => Object.values(choiceActive.value).filter(Boolean).length
);

const sortLabel = computed(() => (sortDirection.value === "asc" ? "↑" : "↓"));

const switchTab = (tab: "order" | "reservation") => {
  activeTab.value = tab;
  searchQuery.value = "";
  openSearch.value = false;
  editTab.value = false;
  choiceActive.value = {};
};

const confirmSearch = () => {
  const keyword = searchQuery.value.trim();
  if (keyword) {
    const existingIndex = historyOrders.value.findIndex(
      (item) => item.name === keyword
    );

    if (existingIndex !== -1) {
      const [item] = historyOrders.value.splice(existingIndex, 1);
      historyOrders.value.unshift(item);
    } else {
      historyOrders.value.unshift({
        id: Date.now(),
        name: keyword,
      });

      if (historyOrders.value.length > MAX_HISTORY_COUNT) {
        historyOrders.value.pop();
      }
    }

    localStorage.setItem("searchHistory", JSON.stringify(historyOrders.value));
  }

  openSearch.value = false;
};

const clearSearchHistory = () => {
  historyOrders.value = [];
  localStorage.removeItem("searchHistory");
};

const loadSearchHistory = () => {
  const savedHistory = localStorage.getItem("searchHistory");
  if (savedHistory) {
    try {
      historyOrders.value = JSON.parse(savedHistory);
    } catch {
      historyOrders.value = [];
    }
  }
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
  confirmSearch();
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
  const existing = localStorage.getItem("userOrderFavorites");
  const favorites = existing
    ? (JSON.parse(existing) as SearchableOrderItem[])
    : [];
  const merged = [...favorites];

  selected.forEach((item) => {
    if (!merged.some((favorite) => favorite.id === item.id)) {
      merged.push(item);
    }
  });

  localStorage.setItem("userOrderFavorites", JSON.stringify(merged));
  choiceActive.value = {};
  editTab.value = false;
};

const deleteSelected = () => {
  const selectedIds = Object.entries(choiceActive.value)
    .filter(([, checked]) => checked)
    .map(([id]) => Number(id));

  if (activeTab.value === "order") {
    orders.value = orders.value.filter(
      (item) => !selectedIds.includes(item.id)
    );
  } else {
    reservationOrder.value = reservationOrder.value.filter(
      (item) => !selectedIds.includes(item.id)
    );
  }

  choiceActive.value = {};
  editTab.value = false;
};

const goToDetail = (item: SearchableOrderItem) => {
  if (editTab.value) {
    ordersButton(item.id);
  } else {
    sessionStorage.setItem(
      "userOrderDetailPreview",
      JSON.stringify({
        ...item,
        tab: activeTab.value,
      })
    );

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

const formatTimeValue = (value?: number) => {
  if (typeof value !== "number") return "待同步";
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return String(value);
  return date.toLocaleString("zh-CN", {
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
};

onMounted(() => {
  document.addEventListener("click", handleClickOutside);
  loadSearchHistory();

  void orderApi
    .getOrderRecords({
      name: store.state.currentUser.userName,
      phone: store.state.currentUser.userPhone,
      email: store.state.currentUser.userEmail,
    })
    .then((records) => {
      orders.value = records;
    });

  void reservationApi
    .getReservationRecords({
      name: store.state.currentUser.userName,
      phone: store.state.currentUser.userPhone,
      email: store.state.currentUser.userEmail,
    })
    .then((records) => {
      reservationOrder.value = records;
    });
});

onBeforeUnmount(() => {
  document.removeEventListener("click", handleClickOutside);
});
</script>

<style scoped lang="scss">
.order-page {
  display: grid;
  grid-template-columns: 260px minmax(0, 1fr);
  gap: 20px;
  height: calc(100vh - 170px);
  min-height: calc(100vh - 170px);
  max-height: calc(100vh - 170px);
  overflow: hidden;
}

.order-sidebar,
.order-stage__toolbar,
.order-summary article,
.order-list-panel,
.batch-actions {
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: rgba(255, 250, 242, 0.82);
  box-shadow: 0 24px 55px rgba(25, 92, 93, 0.08);
}

.order-sidebar {
  position: sticky;
  top: 128px;
  align-self: start;
  display: grid;
  gap: 10px;
  padding: 18px;
}

.order-sidebar__hero {
  display: grid;
  gap: 6px;
  padding: 10px 4px 14px;
}

.order-sidebar__hero p,
.order-list-panel__head p {
  margin: 0;
  color: #1e8a88;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 9px;
  font-weight: 700;
}

.order-sidebar__hero h2,
.order-list-panel__head h3 {
  margin: 0;
  color: #143d40;
  font-size: 25px;
}

.order-sidebar__hero span,
.order-list-panel__head span,
.order-empty span {
  color: #607975;
  line-height: 1.8;
  font-size: 15px;
}

.order-sidebar__item {
  padding: 14px 16px;
  border: 1px solid transparent;
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.58);
  color: #163f42;
  text-align: left;
  font-size: 15px;
  font-weight: 700;
  cursor: pointer;
}

.order-sidebar__item--active {
  border-color: rgba(29, 134, 135, 0.18);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.28),
    rgba(243, 197, 155, 0.22)
  );
  box-shadow: 0 16px 30px rgba(28, 98, 99, 0.1);
}

.order-stage {
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr) auto;
  gap: 22px;
  min-width: 0;
  min-height: 0;
  max-height: 100%;
  overflow: hidden;
}

.order-stage__toolbar {
  position: relative;
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 16px;
  align-items: center;
  height: 80px;
  min-height: 80px;
  max-height: 80px;
  padding: 0 22px;
}

.toolbar-search {
  display: grid;
  grid-template-columns: 56px minmax(0, 1fr);
  align-items: center;
  height: 56px;
  max-height: 56px;
  min-height: 56px;
  border-radius: 20px;
  border: 1px solid rgba(29, 134, 135, 0.1);
  background: rgba(255, 255, 255, 0.64);
  overflow: hidden;
}

.toolbar-search span {
  text-align: center;
  font-size: 14px;
  font-weight: 700;
  color: #187f7d;
}

.toolbar-search input {
  min-width: 0;
  width: calc(80% - 6px);
  min-height: 46px;
  padding: 0 18px 0 8px;
  border: none;
  background: transparent;
  color: #143d40;
  font-size: 16px;
  line-height: 46px;
  outline: none;
}

.search-popover {
  position: absolute;
  top: calc(100% - 6px);
  left: 22px;
  width: min(520px, calc(100% - 44px));
  padding: 16px;
  border-radius: 24px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: rgba(255, 250, 242, 0.96);
  box-shadow: 0 18px 40px rgba(25, 92, 93, 0.1);
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
  color: #173f42;
}

.search-popover__clear {
  border: none;
  background: none;
  color: #cf6d55;
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
  background: rgba(29, 134, 135, 0.08);
  color: #155a5d;
  cursor: pointer;
}

.search-popover__empty {
  margin: 0;
  color: #657d7b;
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
  background: rgba(29, 134, 135, 0.08);
  color: #155a5d;
  white-space: nowrap;
}

.toolbar-edit {
  min-height: 56px;
  height: 56px;
  background: linear-gradient(135deg, #268f90, #156b6b);
  color: #fffdf7;
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
  color: #1e8a88;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.order-summary strong {
  display: block;
  margin-bottom: 6px;
  font-size: 26px;
  color: #143d40;
}

.order-summary span {
  color: #607975;
  font-size: 13px;
  line-height: 1.65;
}

.order-list-panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  align-content: start;
  padding: 22px;
  min-height: 0;
  height: 100%;
  overflow: hidden;
}

.order-list-panel__head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  margin-bottom: 10px;
}

.order-list {
  display: grid;
  align-content: start;
  gap: 14px;
  min-height: 0;
  overflow-y: auto;
  padding-right: 6px;
  scrollbar-width: none;
  -ms-overflow-style: none;
}

.order-list::-webkit-scrollbar {
  display: none;
}

.order-item {
  display: grid;
  grid-template-columns: minmax(0, 1fr);
  gap: 12px;
  align-items: center;
  padding: 16px;
  border-radius: 24px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.64);
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
  accent-color: #1b8585;
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
  color: #153f42;
}

.order-item__main span,
.order-item__meta small {
  color: #69817e;
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
    color: #173f42;
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
  color: #173f42;
  font-weight: 700;
}

.order-item__tag {
  padding: 10px 14px;
  border-radius: 999px;
  background: rgba(29, 134, 135, 0.08);
  color: #166968;
  font-size: 12px;
  font-weight: 700;
}

.order-empty {
  display: grid;
  justify-items: center;
  gap: 8px;
  padding: 34px 18px;
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.58);
  text-align: center;
}

.order-empty strong {
  font-size: 20px;
  color: #163f42;
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
  background: rgba(29, 134, 135, 0.08);
  color: #166968;
}

.batch-actions__danger {
  background: linear-gradient(135deg, #e89a79, #d46f58);
  color: #fffaf6;
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
